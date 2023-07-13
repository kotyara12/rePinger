#include <stdlib.h>
#include <stdbool.h>
#include "project_config.h"
#include "def_consts.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/opt.h"
#include "lwip/init.h"
#include "lwip/mem.h"
#include "lwip/icmp.h"
#include "lwip/dns.h"
#include "lwip/netif.h"
#include "lwip/sys.h"
#include "lwip/timeouts.h"
#include "lwip/inet.h"
#include "lwip/inet_chksum.h"
#include "lwip/ip.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "rLog.h"
#include "rePinger.h"
#include "reEvents.h"
#include "reWiFi.h"
#include "reEsp32.h"
#include "rStrings.h"
#include "reParams.h"
#if CONFIG_MQTT_PINGER_ENABLE
#include "rePingerMqtt.h"
#endif // CONFIG_MQTT_PINGER_ENABLE
#if CONFIG_OPENMON_ENABLE && CONFIG_OPENMON_PINGER_ENABLE
#include "rePingerOM.h"
#endif // CONFIG_OPENMON_ENABLE && CONFIG_OPENMON_PINGER_ENABLE

static const char* logTAG = "PING";
static const char* pingerTaskName = "pinger";

static const uint8_t PING_START = BIT0;
static const uint8_t PING_STOP  = BIT1;

#define PING_CHECK(a, str, goto_tag, ret_value, ...)    \
  do {                                                  \
    if (!(a)) {                                         \
      rlog_e(logTAG, str, ##__VA_ARGS__);               \
      ret = ret_value;                                  \
      goto goto_tag;                                    \
    }                                                   \
  } while (0)

#define PING_SET_MIN(value_a, min_a, value_b, limit_b) if ((value_a < min_a) & (value_b < limit_b)) { min_a = value_a; }
#define PING_SET_MAX(value_a, max_b) if (value_a > max_b) { max_b = value_a; }

#define PING_TIME_DIFF_MS(_end, _start) ((uint32_t)(((_end).tv_sec - (_start).tv_sec) * 1000 + ((_end).tv_usec - (_start).tv_usec) / 1000))

typedef struct {
    const char* host_name;
    ip_addr_t host_addr;
    TickType_t host_resolved;
    int sock;
    struct sockaddr_storage target_addr;
    struct icmp_echo_hdr *packet_hdr;
    uint32_t icmp_pkt_size;
    uint32_t transmitted;
    uint32_t received;
    uint32_t elapsed_time_ms;
    uint32_t total_time_ms;
    uint32_t total_duration_ms;
    float total_loss;
    uint8_t tos;
    uint8_t ttl;
    ping_state_t total_state;
    uint32_t limit_unavailable;
    uint32_t count_unavailable;
    time_t time_unavailable;
    bool notify_unavailable; 
} pinger_data_t;

TaskHandle_t _pingTask;
static uint32_t _pingFlags = 0;

#if CONFIG_PINGER_TASK_STATIC_ALLOCATION
StaticTask_t _pingTaskBuffer;
StackType_t _pingTaskStack[CONFIG_PINGER_TASK_STACK_SIZE];
#endif // CONFIG_PINGER_TASK_STATIC_ALLOCATION

static uint8_t _pingCount = CONFIG_PINGER_PARAM_COUNT;
static uint16_t _pingTimeout = CONFIG_PINGER_PARAM_TIMEOUT;
static uint8_t _pingPacket = CONFIG_PINGER_PARAM_DATASIZE;
static uint8_t _resultMode = CONFIG_PINGER_TOTAL_RESULT_MODE;
static uint32_t _maxSlowdownDuration = CONFIG_PINGER_SLOWDOWN_DURATION;
static float _maxSlowdownLoss = CONFIG_PINGER_SLOWDOWN_LOSS;
static uint32_t _maxUnavailableDuration = CONFIG_PINGER_UNAVAILABLE_DURATION;
static float _maxUnavailableLoss = CONFIG_PINGER_UNAVAILABLE_LOSS;
static uint8_t _thresholdUnavailable = CONFIG_PINGER_UNAVAILABLE_THRESHOLD;
static uint32_t _intervalAvailable = CONFIG_PINGER_INTERVAL_AVAILABLE;
static uint32_t _intervalUnavailable = CONFIG_PINGER_INTERVAL_UNAVAILABLE;

static void pingerParamsRegister()
{
  paramsGroupHandle_t pgPinger = paramsRegisterGroup(nullptr, 
    CONFIG_PINGER_PGROUP_ROOT_KEY, CONFIG_PINGER_PGROUP_ROOT_TOPIC, CONFIG_PINGER_PGROUP_ROOT_FRIENDLY);
  
  paramsSetLimitsU8(
    paramsRegisterValue(OPT_KIND_PARAMETER, OPT_TYPE_U8, nullptr, pgPinger,
      CONFIG_PINGER_PARAM_COUNT_KEY, CONFIG_PINGER_PARAM_COUNT_FRIENDLY,
      CONFIG_MQTT_PARAMS_QOS, (void*)&_pingCount),
    1, 25);
  paramsSetLimitsU16(
    paramsRegisterValue(OPT_KIND_PARAMETER, OPT_TYPE_U16, nullptr, pgPinger,
      CONFIG_PINGER_PARAM_TIMEOUT_KEY, CONFIG_PINGER_PARAM_TIMEOUT_FRIENDLY,
      CONFIG_MQTT_PARAMS_QOS, (void*)&_pingTimeout),
    100, 60000);
  paramsSetLimitsU8(
    paramsRegisterValue(OPT_KIND_PARAMETER, OPT_TYPE_U8, nullptr, pgPinger,
      CONFIG_PINGER_PARAM_DATASIZE_KEY, CONFIG_PINGER_PARAM_DATASIZE_FRIENDLY,
      CONFIG_MQTT_PARAMS_QOS, (void*)&_pingPacket),
    1, 255);

  paramsSetLimitsU8(
    paramsRegisterValue(OPT_KIND_PARAMETER, OPT_TYPE_U8, nullptr, pgPinger,
      CONFIG_PINGER_PARAM_RESULT_MODE_KEY, CONFIG_PINGER_PARAM_RESULT_MODE_FRIENDLY,
      CONFIG_MQTT_PARAMS_QOS, (void*)&_resultMode),
    0, 2);

  paramsRegisterValue(OPT_KIND_PARAMETER, OPT_TYPE_U32, nullptr, pgPinger,
    CONFIG_PINGER_PARAM_SLOWDOWN_DURATION_KEY, CONFIG_PINGER_PARAM_SLOWDOWN_DURATION_FRIENDLY,
    CONFIG_MQTT_PARAMS_QOS, (void*)&_maxSlowdownDuration);
  paramsSetLimitsFloat(
    paramsRegisterValue(OPT_KIND_PARAMETER, OPT_TYPE_FLOAT, nullptr, pgPinger,
      CONFIG_PINGER_PARAM_SLOWDOWN_LOSS_KEY, CONFIG_PINGER_PARAM_SLOWDOWN_LOSS_FRIENDLY,
      CONFIG_MQTT_PARAMS_QOS, (void*)&_maxSlowdownLoss),
    0, 100);

  paramsRegisterValue(OPT_KIND_PARAMETER, OPT_TYPE_U32, nullptr, pgPinger,
    CONFIG_PINGER_PARAM_UNAVAILABLE_DURATION_KEY, CONFIG_PINGER_PARAM_UNAVAILABLE_DURATION_FRIENDLY,
    CONFIG_MQTT_PARAMS_QOS, (void*)&_maxUnavailableDuration);
  paramsSetLimitsFloat(
    paramsRegisterValue(OPT_KIND_PARAMETER, OPT_TYPE_FLOAT, nullptr, pgPinger,
      CONFIG_PINGER_PARAM_UNAVAILABLE_LOSS_KEY, CONFIG_PINGER_PARAM_UNAVAILABLE_LOSS_FRIENDLY,
      CONFIG_MQTT_PARAMS_QOS, (void*)&_maxUnavailableLoss),
    0, 100);

  paramsRegisterValue(OPT_KIND_PARAMETER, OPT_TYPE_U8, nullptr, pgPinger,
    CONFIG_PINGER_PARAM_UNAVAILABLE_THRESHOLD_KEY, CONFIG_PINGER_PARAM_UNAVAILABLE_THRESHOLD_FRIENDLY,
    CONFIG_MQTT_PARAMS_QOS, (void*)&_thresholdUnavailable);

  paramsSetLimitsU32(
    paramsRegisterValue(OPT_KIND_PARAMETER, OPT_TYPE_U32, nullptr, pgPinger,
      CONFIG_PINGER_PARAM_INTERVAL_AVAILABLE_KEY, CONFIG_PINGER_PARAM_INTERVAL_AVAILABLE_FRIENDLY,
      CONFIG_MQTT_PARAMS_QOS, (void*)&_intervalAvailable),
    1000, 3600000);
  paramsSetLimitsU32(
    paramsRegisterValue(OPT_KIND_PARAMETER, OPT_TYPE_U32, nullptr, pgPinger,
      CONFIG_PINGER_PARAM_INTERVAL_UNAVAILABLE_KEY, CONFIG_PINGER_PARAM_INTERVAL_UNAVAILABLE_FRIENDLY,
      CONFIG_MQTT_PARAMS_QOS, (void*)&_intervalUnavailable),
    1000, 3600000);
}

static esp_err_t pingerSend(pinger_data_t *ep)
{
  esp_err_t ret = ESP_OK;
  ep->packet_hdr->seqno++;
  // Generate checksum since "seqno" has changed 
  ep->packet_hdr->chksum = 0;
  if (ep->packet_hdr->type == ICMP_ECHO) {
    ep->packet_hdr->chksum = inet_chksum(ep->packet_hdr, ep->icmp_pkt_size);
  }

  ssize_t sent = sendto(ep->sock, ep->packet_hdr, ep->icmp_pkt_size, 0,
    (struct sockaddr*)&ep->target_addr, sizeof(ep->target_addr));

  if (sent != (ssize_t)ep->icmp_pkt_size) {
    int opt_val;
    socklen_t opt_len = sizeof(opt_val);
    getsockopt(ep->sock, SOL_SOCKET, SO_ERROR, &opt_val, &opt_len);
    rlog_e(logTAG, "Send ICMP error = %d", opt_val);
    ret = ESP_FAIL;
  } else {
    ep->transmitted++;
  }
  return ret;
}

static int pingerReceive(pinger_data_t *ep)
{
  char buf[64]; // 64 bytes are enough to cover IP header and ICMP header
  int len = 0;
  struct sockaddr_storage from;
  int fromlen = sizeof(from);
  uint16_t data_head = 0;

  while ((len = recvfrom(ep->sock, buf, sizeof(buf), 0, (struct sockaddr *)&from, (socklen_t *)&fromlen)) > 0) {
    if (from.ss_family == AF_INET) {
      // IPv4
      struct sockaddr_in *from4 = (struct sockaddr_in *)&from;
      inet_addr_to_ip4addr(ip_2_ip4(&ep->host_addr), &from4->sin_addr);
      IP_SET_TYPE_VAL(ep->host_addr, IPADDR_TYPE_V4);
      data_head = (uint16_t)(sizeof(struct ip_hdr) + sizeof(struct icmp_echo_hdr));
    #if CONFIG_LWIP_IPV6
    } else {
      // IPv6
      struct sockaddr_in6 *from6 = (struct sockaddr_in6 *)&from;
      inet6_addr_to_ip6addr(ip_2_ip6(&ep->host_addr), &from6->sin6_addr);
      IP_SET_TYPE_VAL(ep->host_addr, IPADDR_TYPE_V6);
      data_head = (uint16_t)(sizeof(struct ip6_hdr) + sizeof(struct icmp6_echo_hdr));
    #endif // CONFIG_LWIP_IPV6
    };

    if (len >= data_head) {
      if (IP_IS_V4_VAL(ep->host_addr)) {              
        // Currently we process IPv4
        struct ip_hdr *iphdr = (struct ip_hdr *)buf;
        struct icmp_echo_hdr *iecho = (struct icmp_echo_hdr *)(buf + (IPH_HL(iphdr) * 4));
        if ((iecho->id == ep->packet_hdr->id) && (iecho->seqno == ep->packet_hdr->seqno)) {
          ep->received++;
          ep->ttl = iphdr->_ttl;
          // ep->recv_len = lwip_ntohs(IPH_LEN(iphdr)) - data_head;  // The data portion of ICMP
          return len;
        }
      #if CONFIG_LWIP_IPV6
      } else if (IP_IS_V6_VAL(ep->host_addr)) {      
        // Currently we process IPv6
        struct ip6_hdr *iphdr = (struct ip6_hdr *)buf;
        struct icmp6_echo_hdr *iecho6 = (struct icmp6_echo_hdr *)(buf + sizeof(struct ip6_hdr)); // IPv6 head length is 40
        if ((iecho6->id == ep->packet_hdr->id) && (iecho6->seqno == ep->packet_hdr->seqno)) {
          ep->received++;
          // ep->recv_len = IP6H_PLEN(iphdr) - sizeof(struct icmp6_echo_hdr); // The data portion of ICMPv6
          return len;
        }
      #endif // CONFIG_LWIP_IPV6
      };
    };

    fromlen = sizeof(from);
  }
  // if timeout, len will be -1
  return len;
}

static void pingerFreeSession(pinger_data_t *ep)
{
  if (ep) {
    if (ep->sock > 0) {
      lwip_close(ep->sock);
      ep->sock = 0;
    };
    ep->host_resolved = 0;
    ip_addr_set_zero(&ep->host_addr);
    if (ep->packet_hdr) {
      free(ep->packet_hdr);
      ep->packet_hdr = nullptr;
    };
  }
}

static esp_err_t pingerInitSession(pinger_data_t *ep, const char* hostname, uint32_t hostid, uint32_t limit_unavailable)
{
  esp_err_t ret = ESP_OK;
  PING_CHECK(ep, "Ping data can't be null", err, ESP_ERR_INVALID_ARG);
  memset(ep, 0, sizeof(pinger_data_t));

  // Set parameters for ping
  ep->host_name = hostname;
  ep->host_resolved = 0;
  ip_addr_set_zero(&ep->host_addr);
  ep->total_state = PING_OK;
  ep->limit_unavailable = limit_unavailable;
  ep->count_unavailable = 0;
  ep->notify_unavailable = false;

  // Allocating memory for a data packet
  ep->icmp_pkt_size = sizeof(struct icmp_echo_hdr) + _pingPacket;
  ep->packet_hdr = (icmp_echo_hdr*)esp_calloc(1, ep->icmp_pkt_size);
  PING_CHECK(ep->packet_hdr, "No memory for echo packet", err, ESP_ERR_NO_MEM);
  
  // Set ICMP type and code field
  ep->packet_hdr->id = hostid;
  ep->packet_hdr->code = 0;
  // Fill the additional data buffer with some data
  {
    char *d = (char*)ep->packet_hdr + sizeof(struct icmp_echo_hdr);
    for (uint32_t i = 0; i < _pingPacket; i++) {
      d[i] = 'A' + i;
    };
  }
  return ret;
err:
  if (ep->packet_hdr) {
    free(ep->packet_hdr);
    ep->packet_hdr = nullptr;
  };
  return ret;
}

static void pingerCloseSocket(pinger_data_t *ep)
{
  if (ep) {
    if (ep->sock > 0) {
      lwip_close(ep->sock);
      ep->sock = 0;
    };
  }
}

#if LWIP_DNS
static void pingerDnsFound(const char* hostname, const ip_addr_t *ipaddr, void *arg)
{
  pinger_data_t *ep = (pinger_data_t *)arg;

  if (ipaddr == nullptr) {
    ep->host_resolved = 0;
    ip_addr_set_zero(&ep->host_addr);
  } else {
    ep->host_resolved = xTaskGetTickCount();
    ep->host_addr = *ipaddr;
  };
}
#endif /* LWIP_DNS */ 

// Resolve hostname to IP address
static esp_err_t pingerResolveName(pinger_data_t *ep)
{
  esp_err_t ret = ESP_OK;

  ep->host_resolved = 0;
  ip_addr_set_zero(&ep->host_addr);

  #if LWIP_DNS
    ret = dns_gethostbyname(ep->host_name, &ep->host_addr, pingerDnsFound, ep);
  #else
    ret = ipaddr_aton(ep->host_name, &ep->host_addr) ? ERR_OK : ERR_ARG;
  #endif // LWIP_DNS
  
  if (ret == ERR_INPROGRESS) {
    uint16_t waitTicks = pdMS_TO_TICKS(10000);
    while ((waitTicks > 0) && (ep->host_resolved == 0)) {
      vTaskDelay(1);
      waitTicks--;
    };
    if (ep->host_resolved == 0) {
      rlog_e(logTAG, "Failed to resolve a hostname [ %s ]: DNS TIMEOUT", ep->host_name);
      return ESP_ERR_NOT_FOUND;
    } else {
      ret = ESP_OK;
    };
  };

  if (ret == ESP_OK) {
    #if CONFIG_LWIP_IPV6
      // todo: IPV6 log support
      if (IP_IS_V4(&ep->host_addr)) {
        rlog_d(logTAG, "IP address obtained for hostname [ %s ]: %d.%d.%d.%d", 
          ep->host_name, 
          ip4_addr1(&ep->host_addr.u_addr.ip4),
          ip4_addr2(&ep->host_addr.u_addr.ip4),
          ip4_addr3(&ep->host_addr.u_addr.ip4),
          ip4_addr4(&ep->host_addr.u_addr.ip4));
      };
    #else
      rlog_d(logTAG, "IP address obtained for hostname [ %s ]: %d.%d.%d.%d", 
        ep->host_name, 
        ip4_addr1(&ep->host_addr),
        ip4_addr2(&ep->host_addr),
        ip4_addr3(&ep->host_addr),
        ip4_addr4(&ep->host_addr));
    #endif // CONFIG_LWIP_IPV6
  } else {
    rlog_e(logTAG, "Failed to resolve a hostname [ %s ]: %d %s", ep->host_name, ret, esp_err_to_name(ret));
    return ESP_ERR_NOT_FOUND;
  };
  
  return ret;
}

static esp_err_t pingerOpenSocket(pinger_data_t *ep)
{
  esp_err_t ret = ESP_OK;
  PING_CHECK(ep, "Ping data can't be null", err, ESP_ERR_INVALID_ARG);

  // Resolve hostname to IP address, if needed
  if (ep->host_resolved == 0) {
    ret = pingerResolveName(ep);
    if (ret != ESP_OK) return ret;
  };

  // Create socket
  #if CONFIG_LWIP_IPV6
    if (IP_IS_V4(&ep->host_addr) || ip6_addr_isipv4mappedipv6(ip_2_ip6(&ep->host_addr))) {
      ep->sock = lwip_socket(AF_INET, SOCK_RAW, IP_PROTO_ICMP);
    } else {
      ep->sock = lwip_socket(AF_INET6, SOCK_RAW, IP6_NEXTH_ICMP6);
    };
  #else
    ep->sock = lwip_socket(AF_INET, SOCK_RAW, IP_PROTO_ICMP);
  #endif // CONFIG_LWIP_IPV6

  PING_CHECK(ep->sock > 0, "Create socket failed: %d", err, ESP_FAIL, ep->sock);

  // Set receive timeout
  struct timeval timeout;
  timeout.tv_sec = _pingTimeout / 1000;
  timeout.tv_usec = (_pingTimeout % 1000) * 1000;
  setsockopt(ep->sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

  // Set tos
  setsockopt(ep->sock, IPPROTO_IP, IP_TOS, &ep->tos, sizeof(ep->tos));

  // Set socket address
  if (IP_IS_V4(&ep->host_addr)) {
    struct sockaddr_in *to4 = (struct sockaddr_in *)&ep->target_addr;
    to4->sin_family = AF_INET;
    inet_addr_from_ip4addr(&to4->sin_addr, ip_2_ip4(&ep->host_addr));
    ep->packet_hdr->type = ICMP_ECHO;
  };
  #if CONFIG_LWIP_IPV6
    if (IP_IS_V6(&ep->host_addr)) {
      struct sockaddr_in6 *to6 = (struct sockaddr_in6 *)&ep->target_addr;
      to6->sin6_family = AF_INET6;
      inet6_addr_from_ip6addr(&to6->sin6_addr, ip_2_ip6(&ep->host_addr));
      ep->packet_hdr->type = ICMP6_TYPE_EREQ;
    };
  #endif // CONFIG_LWIP_IPV6
  return ESP_OK;
err:
  pingerCloseSocket(ep);
  return ret;
}

static void pingerCopyHostData(pinger_data_t *ep, ping_host_data_t* host_data)
{
  memset(host_data, 0, sizeof(ping_host_data_t));
  host_data->host_name = ep->host_name;
  host_data->host_addr = ep->host_addr;
  host_data->transmitted = ep->transmitted;
  host_data->received = ep->received;
  host_data->total_time_ms = ep->total_time_ms;
  host_data->duration_ms = ep->total_duration_ms;
  host_data->loss = ep->total_loss;
  host_data->ttl = ep->ttl;
  host_data->state = ep->total_state;
}

static ping_state_t pingerCheckHostEx(pinger_data_t *ep)
{
  #if CONFIG_PING_SHOW_INTERMEDIATE
  rlog_d(logTAG, "Ping host [ %s ]...", ep->host_name);
  #endif // CONFIG_PING_SHOW_INTERMEDIATE

  // Resolve hostname to IP address
  if ( (ep->received == 0) 
    || (ep->host_resolved == 0) 
    || ((xTaskGetTickCount() - ep->host_resolved) > pdMS_TO_TICKS(CONFIG_PINGER_IP_VALIDITY)) ) {
    pingerCloseSocket(ep);
    esp_err_t err = pingerResolveName(ep);
    if (err != ESP_OK) goto err;
  };

  // Opening socket
  if (ep->sock <= 0) {
    if (pingerOpenSocket(ep) != ESP_OK) goto err;
    if (ep->sock <= 0) goto err;
  };

  // Initialize runtime statistics
  ep->packet_hdr->seqno = 0;
  ep->transmitted = 0;
  ep->received = 0;
  ep->total_time_ms = 0;
  ep->total_duration_ms = 0;
  ep->total_loss = 0;

  // Batch of ping operations
  struct timeval timeSend, timeEnd;
  for (uint32_t i = 0; i < _pingCount; i++) {
    // Send packet
    esp_err_t send_ret = pingerSend(ep);
    gettimeofday(&timeSend, NULL);
    if (send_ret != ESP_OK) goto err;

    // Recieve response
    int recv_ret = pingerReceive(ep);
    gettimeofday(&timeEnd, NULL);
    ep->elapsed_time_ms = PING_TIME_DIFF_MS(timeEnd, timeSend);
    if (ep->elapsed_time_ms > 1000000000) {
      ep->elapsed_time_ms = rand() % _pingTimeout;
    };
    ep->total_time_ms += ep->elapsed_time_ms;

    #if CONFIG_PING_SHOW_INTERMEDIATE
    if (recv_ret >= 0) {
      rlog_d(logTAG, "Received of %d bytes from [%s : %s]: icmp_seq = %d, ttl = %d, time = %d ms",
        ep->datasize, ep->host_name, ipaddr_ntoa(&ep->host_addr), ep->packet_hdr->seqno, ep->ttl, ep->elapsed_time_ms);
    } else {
      rlog_w(logTAG, "Packet loss for [%s : %s]: icmp_seq = %d", 
        ep->host_name, ipaddr_ntoa(&ep->host_addr), ep->packet_hdr->seqno);
    };
    #endif // CONFIG_PING_SHOW_INTERMEDIATE
  };

  // Calculating loss and average response time
  if (ep->transmitted > 0) {
    ep->total_duration_ms = ep->total_time_ms / ep->transmitted;
    ep->total_loss = (float)((1 - ((float)ep->received) / ep->transmitted) * 100);
    if (ep->received == 0) {
      ep->total_state = PING_UNAVAILABLE;
    } else {
      ep->total_state = PING_OK;
    };
  } else goto err;

  // Close socket
  #if CONFIG_PING_KEEP_SOCKET == 0
  pingerCloseSocket(ep);
  #endif // CONFIG_PING_KEEP_SOCKET

  // Returning the batch result
  return ep->total_state;
err:
  ep->total_duration_ms = _pingTimeout;
  ep->total_loss = 100.0;
  ep->total_state = PING_FAILED;
  pingerCloseSocket(ep);
  return ep->total_state;
}


static ping_state_t pingerCheckHost(pinger_data_t *ep, re_ping_event_id_t evid_availavble, re_ping_event_id_t evid_unavailavble)
{
  // Ping host
  pingerCheckHostEx(ep);

  // Show log
  rlog_d(logTAG, "Ping statistics for [%s : %s]: %d packets transmitted, %d received, %.1f% % packet loss, average time %d ms",
    ep->host_name, ipaddr_ntoa(&ep->host_addr), ep->transmitted, ep->received, ep->total_loss, ep->total_duration_ms);

  // Copy results to data to send to event loop
  ping_host_data_t host_data;
  pingerCopyHostData(ep, &host_data);
  
  // Post event
  if (ep->total_state == PING_OK) {
    if (ep->notify_unavailable || (ep->count_unavailable > 0)) {
      rlog_i(logTAG, "Host [ %s ] is available", ep->host_name);
      host_data.time_unavailable = ep->time_unavailable;
      ep->count_unavailable = 0;
      ep->time_unavailable = 0;
      if (ep->notify_unavailable) {
        ep->notify_unavailable = false;
        eventLoopPost(RE_PING_EVENTS, evid_availavble, &host_data, sizeof(host_data), portMAX_DELAY);
      };
    };
  } else {
    if (ep->time_unavailable == 0) {
      ep->time_unavailable = time(nullptr);
    };
    ep->count_unavailable++;
    rlog_w(logTAG, "Host [ %s ] is not available (count=%d)", ep->host_name, ep->count_unavailable);
    if ((!ep->notify_unavailable) && (ep->count_unavailable >= ep->limit_unavailable)) {
      ep->notify_unavailable = true;
      host_data.time_unavailable = ep->time_unavailable;
      eventLoopPost(RE_PING_EVENTS, evid_unavailavble, &host_data, sizeof(host_data), portMAX_DELAY);
    };
  };
  
  return ep->total_state;
}

#define LOGMSG_SERVICE_STARTED "Service access check Internet access was started"
#define LOGMSG_SERVICE_STOPPED "Service access check Internet access was stopped"

static void pingerExec(void *args)
{
  static ping_publish_data_t data;
  memset(&data, 0, sizeof(data));
  data.inet.state = PING_FAILED;
  data.inet.time_unavailable = 0;
  static bool pingEnabled = true;
  static bool pingLastOk = true;
  static BaseType_t waitResult;
  static uint32_t waitFlags;
  static TickType_t lastCheck = 0;
  static TickType_t waitTicks = 0;
  ping_state_t inet_state;
  #if (CONFIG_PINGER_FILTER_MODE > 0) && (CONFIG_PINGER_FILTER_SIZE > 0)
  static bool bufReset = true;
  static uint8_t bufIndex = 0;
  static uint16_t bufDuration[CONFIG_PINGER_FILTER_SIZE];
  #endif // CONFIG_PINGER_FILTER_MODE

  pingerParamsRegister();
  
  #ifdef CONFIG_PINGER_HOST_1  
    static pinger_data_t pdHost1;
    if (pingerInitSession(&pdHost1, CONFIG_PINGER_HOST_1, 7001, 1) == ESP_OK) { data.inet.hosts_count++; };
  #endif // CONFIG_PINGER_HOST_1  
  #ifdef CONFIG_PINGER_HOST_2
    static pinger_data_t pdHost2;
    if (pingerInitSession(&pdHost2, CONFIG_PINGER_HOST_2, 7002, 1) == ESP_OK) { data.inet.hosts_count++; };
  #endif // CONFIG_PINGER_HOST_2
  #ifdef CONFIG_PINGER_HOST_3
    static pinger_data_t pdHost3;
    if (pingerInitSession(&pdHost3, CONFIG_PINGER_HOST_3, 7003, 1) == ESP_OK) { data.inet.hosts_count++; };
  #endif // CONFIG_PINGER_HOST_3

  #if CONFIG_MQTT1_PING_CHECK
    static pinger_data_t pdMqtt1;
    pingerInitSession(&pdMqtt1, CONFIG_MQTT1_HOST, 8101, CONFIG_MQTT1_PING_CHECK_LIMIT);
  #endif // CONFIG_MQTT1_PING_CHECK
  #if CONFIG_MQTT2_PING_CHECK
    pinger_data_t pdMqtt2;
    pingerInitSession(&pdMqtt2, CONFIG_MQTT2_HOST, 8102, CONFIG_MQTT2_PING_CHECK_LIMIT);
  #endif // CONFIG_MQTT2_PING_CHECK

  #if CONFIG_OPENMON_ENABLE && CONFIG_OPENMON_PINGER_ENABLE
    pingerOpenMonInit();
  #endif // CONFIG_OPENMON_ENABLE && CONFIG_OPENMON_PINGER_ENABLE

  // Posting an event
  rlog_i(logTAG, LOGMSG_SERVICE_STARTED);
  eventLoopPost(RE_PING_EVENTS, RE_PING_STARTED, nullptr, 0, portMAX_DELAY);

  while (1) {
    // Waiting for task start or pause notifications
    waitResult = xTaskNotifyWait(0, ULONG_MAX, &waitFlags, waitTicks);
    if (waitResult == pdPASS) {
      if ((waitFlags & PING_START) == PING_START) {
        if (!pingEnabled) {
          pingEnabled = true;
          rlog_i(logTAG, LOGMSG_SERVICE_STARTED);
          eventLoopPost(RE_PING_EVENTS, RE_PING_STARTED, nullptr, 0, portMAX_DELAY);
          data.inet.state = PING_FAILED;
        };
      } else if ((waitFlags & PING_STOP) == PING_STOP) {
        if (pingEnabled) {
          pingEnabled = false;
          rlog_i(logTAG, LOGMSG_SERVICE_STOPPED);
          eventLoopPost(RE_PING_EVENTS, RE_PING_STOPPED, nullptr, 0, portMAX_DELAY);
        };
        waitTicks = portMAX_DELAY;
      }
    };

    // Performing pings
    if (pingEnabled) {
      rlog_i(logTAG, "Internet access is checked...");
      lastCheck = xTaskGetTickCount();

      // Check hosts
      data.inet.hosts_available = 0;
      data.inet.duration_ms_min = 0;
      data.inet.duration_ms_max = 0;
      data.inet.loss_min = 0;
      data.inet.loss_max = 0;

      // Host 1
      if (pingerCheckHost(&pdHost1, RE_PING_HOST_AVAILABLE, RE_PING_HOST_UNAVAILABLE) < PING_UNAVAILABLE) {
        data.inet.hosts_available++;
      };
      data.inet.duration_ms_min = pdHost1.total_duration_ms;
      data.inet.duration_ms_max = pdHost1.total_duration_ms;
      data.inet.duration_ms_total = pdHost1.total_duration_ms;
      data.inet.loss_min = pdHost1.total_loss;
      data.inet.loss_max = pdHost1.total_loss;
      data.inet.loss_total = pdHost1.total_loss;
      pingerCopyHostData(&pdHost1, &data.host1);

      // Host 2
      #ifdef CONFIG_PINGER_HOST_2
        if (pingerCheckHost(&pdHost2, RE_PING_HOST_AVAILABLE, RE_PING_HOST_UNAVAILABLE) < PING_UNAVAILABLE) {
          data.inet.hosts_available++;
        };
        data.inet.duration_ms_total = (pdHost1.total_duration_ms + pdHost2.total_duration_ms) / 2;
        data.inet.loss_total = (pdHost1.total_loss + pdHost2.total_loss) / 2;
        PING_SET_MIN(pdHost2.total_duration_ms, data.inet.duration_ms_min, pdHost2.total_loss, _maxUnavailableLoss);
        PING_SET_MAX(pdHost2.total_duration_ms, data.inet.duration_ms_max);
        PING_SET_MIN(pdHost2.total_loss, data.inet.loss_min, pdHost2.total_duration_ms, _maxUnavailableDuration);
        PING_SET_MAX(pdHost2.total_loss, data.inet.loss_max);
        pingerCopyHostData(&pdHost2, &data.host2);
      #endif // CONFIG_PINGER_HOST_2

      // Host 3
      #ifdef CONFIG_PINGER_HOST_3
        if (pingerCheckHost(&pdHost3, RE_PING_HOST_AVAILABLE, RE_PING_HOST_UNAVAILABLE) < PING_UNAVAILABLE) {
          data.inet.hosts_available++;
        };
        data.inet.duration_ms_total = (pdHost1.total_duration_ms + pdHost2.total_duration_ms + pdHost3.total_duration_ms) / 3;
        data.inet.loss_total = (pdHost1.total_loss + pdHost2.total_loss + pdHost3.total_loss) / 3;
        PING_SET_MIN(pdHost3.total_duration_ms, data.inet.duration_ms_min, pdHost3.total_loss, _maxUnavailableLoss);
        PING_SET_MAX(pdHost3.total_duration_ms, data.inet.duration_ms_max);
        PING_SET_MIN(pdHost3.total_loss, data.inet.loss_min, pdHost3.total_duration_ms, _maxUnavailableDuration);
        PING_SET_MAX(pdHost3.total_loss, data.inet.loss_max);
        pingerCopyHostData(&pdHost3, &data.host3);
      #endif // CONFIG_PINGER_HOST_3
      
      // Determine the final results by which we will evaluate the status of Internet access
      if (_resultMode == 0) {
        data.inet.duration_ms_total = data.inet.duration_ms_min;
        data.inet.loss_total = data.inet.loss_min;
      }
      else if (_resultMode == 2) {
        data.inet.duration_ms_total = data.inet.duration_ms_max;
        data.inet.loss_total = data.inet.loss_max;
      };

      // Remember unfiltered result
      pingLastOk = ((data.inet.hosts_available > 0) && (data.inet.duration_ms_total < _maxSlowdownDuration) && (data.inet.loss_total < _maxSlowdownLoss));

      // Filter for "smoothing" ping results (0 - disabled, 1 - average, 2 - median)
      #if (CONFIG_PINGER_FILTER_MODE > 0) && (CONFIG_PINGER_FILTER_SIZE > 0)
        if (bufReset) {
          bufReset = false;
          for (uint8_t i = 0; i < CONFIG_PINGER_FILTER_SIZE; i++) {
            bufDuration[i] = data.inet.duration_ms_total;
          };
        } else {
          bufDuration[bufIndex] = data.inet.duration_ms_total;
        };

        #if CONFIG_PINGER_FILTER_MODE == 1
          // Average
          uint32_t sumDuration = 0;
          for (uint16_t i = 0; i < CONFIG_PINGER_FILTER_SIZE; i++) {
            sumDuration += bufDuration[i];
          };
          data.inet.duration_ms_total = (uint16_t)(sumDuration / CONFIG_PINGER_FILTER_SIZE);
        #elif CONFIG_PINGER_FILTER_MODE == 2
          // Median
          if ((bufIndex < CONFIG_PINGER_FILTER_SIZE - 1) && (bufDuration[bufIndex] > bufDuration[bufIndex + 1])) {
            for (int i = bufIndex; i < CONFIG_PINGER_FILTER_SIZE - 1; i++) {
              if (bufDuration[i] > bufDuration[i + 1]) {
                uint16_t buff = bufDuration[i];
                bufDuration[i] = bufDuration[i + 1];
                bufDuration[i + 1] = buff;
              };
            };
          } else {
            if ((bufIndex > 0) && (bufDuration[bufIndex - 1] > bufDuration[bufIndex])) {
              for (int i = bufIndex; i > 0; i--) {
                if (bufDuration[i] < bufDuration[i - 1]) {
                  uint16_t buff = bufDuration[i];
                  bufDuration[i] = bufDuration[i - 1];
                  bufDuration[i - 1] = buff;
                };
              };
            };
          };
          data.inet.duration_ms_total = bufDuration[CONFIG_PINGER_FILTER_SIZE / 2];
        #endif // CONFIG_PINGER_FILTER_MODE

        if (++bufIndex >= CONFIG_PINGER_FILTER_SIZE) bufIndex = 0;
      #endif // CONFIG_PINGER_FILTER_MODE

      // Analyze results and send events
      if ((data.inet.hosts_available > 0) && (data.inet.duration_ms_total < _maxUnavailableDuration) && (data.inet.loss_total <= _maxUnavailableLoss)) {
        // Status analysis by total filtered response time
        if ((data.inet.duration_ms_total < _maxSlowdownDuration) && (data.inet.loss_total < _maxSlowdownLoss)) {
          inet_state = PING_OK;
          rlog_i(logTAG, "Internet access is available (%d ms)", data.inet.duration_ms_total);
          // Posting an event only when the status changes
          if (data.inet.state != inet_state) {
            data.inet.state = inet_state;
            eventLoopPost(RE_PING_EVENTS, RE_PING_INET_AVAILABLE, &data.inet, sizeof(data.inet), portMAX_DELAY);
            data.inet.time_unavailable = 0;
          };
        } else {
          inet_state = PING_SLOWDOWN;
          pingLastOk = false;
          rlog_w(logTAG, "Internet access is slowed (%d ms)", data.inet.duration_ms_total);
          if (data.inet.state != inet_state) {
            if (data.inet.time_unavailable == 0) {
              data.inet.time_unavailable = time(nullptr);
            };
            data.inet.state = inet_state;
            eventLoopPost(RE_PING_EVENTS, RE_PING_INET_SLOWDOWN, &data.inet, sizeof(data.inet), portMAX_DELAY);
          };
        };
        data.inet.count_unavailable = 0;
      } else {
        // Failed to reach any of the hosts
        inet_state = PING_UNAVAILABLE;
        pingLastOk = false;
        rlog_e(logTAG, "Internet access is not available!");
        if (data.inet.state != inet_state) {
          data.inet.count_unavailable++;
          if ((data.inet.state <= PING_UNAVAILABLE) || (data.inet.time_unavailable == 0)) {
            data.inet.time_unavailable = time(nullptr);
          };
          if ((data.inet.state == PING_OK) || (data.inet.state == PING_SLOWDOWN)) {
            if (data.inet.count_unavailable >= _thresholdUnavailable) {
              data.inet.state = PING_UNAVAILABLE;
              eventLoopPost(RE_PING_EVENTS, RE_PING_INET_UNAVAILABLE, &data.inet, sizeof(data.inet), portMAX_DELAY);
            };
          } else {
            data.inet.state = PING_UNAVAILABLE;
          };
        };
      };

      // Publishing server check results
      #if CONFIG_MQTT_PINGER_ENABLE
      pingerMqttPublish(&data);
      #endif // CONFIG_MQTT_PINGER_ENABLE
      #if CONFIG_OPENMON_ENABLE && CONFIG_OPENMON_PINGER_ENABLE
      pingerOpenMonPublish(&data);
      #endif // CONFIG_OPENMON_ENABLE && CONFIG_OPENMON_PINGER_ENABLE

      // Additional checks for individual hosts
      if (pingLastOk) {
        #if CONFIG_MQTT1_PING_CHECK
        pingerCheckHost(&pdMqtt1, RE_PING_MQTT1_AVAILABLE, RE_PING_MQTT1_UNAVAILABLE);
        #endif // CONFIG_MQTT1_PING_CHECK
        #if CONFIG_MQTT2_PING_CHECK
        pingerCheckHost(&pdMqtt2, RE_PING_MQTT2_AVAILABLE, RE_PING_MQTT2_UNAVAILABLE);
        #endif // CONFIG_MQTT2_PING_CHECK
      };

      // Waiting interval between periodic checks
      if (pingLastOk) {
        waitTicks = (xTaskGetTickCount() - lastCheck);
        if (pdMS_TO_TICKS(_intervalAvailable) > waitTicks) {
          waitTicks = pdMS_TO_TICKS(_intervalAvailable) - waitTicks;
        } else {
          waitTicks = 1;
        };
      } else {
        waitTicks = pdMS_TO_TICKS(_intervalUnavailable);
      };
    };
  };

  // Before exit task, free all resources
  pingerFreeSession(&pdHost1);
  #ifdef CONFIG_PINGER_HOST_2
  pingerFreeSession(&pdHost2);
  #endif // CONFIG_PINGER_HOST_2
  #ifdef CONFIG_PINGER_HOST_3
  pingerFreeSession(&pdHost3);
  #endif // CONFIG_PINGER_HOST_3

  #if CONFIG_MQTT1_PING_CHECK
  pingerFreeSession(&pdMqtt1);
  #endif // CONFIG_MQTT1_PING_CHECK
  #if CONFIG_MQTT2_PING_CHECK
  pingerFreeSession(&pdMqtt2);
  #endif // CONFIG_MQTT2_PING_CHECK
  
  // Delete task
  vTaskDelete(NULL);
}

// -----------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------- Task routines ----------------------------------------------------
// -----------------------------------------------------------------------------------------------------------------------

bool pingerTaskCreate(bool createSuspended)
{
  if (!_pingTask) {
    #if CONFIG_PINGER_TASK_STATIC_ALLOCATION
      _pingTask = xTaskCreateStaticPinnedToCore(pingerExec, pingerTaskName, 
        CONFIG_PINGER_TASK_STACK_SIZE, NULL, CONFIG_TASK_PRIORITY_PINGER, 
        _pingTaskStack, &_pingTaskBuffer, 
        CONFIG_TASK_CORE_PINGER); 
    #else
      xTaskCreatePinnedToCore(pingerExec, pingerTaskName, 
        CONFIG_PINGER_TASK_STACK_SIZE, NULL, CONFIG_TASK_PRIORITY_PINGER, 
        &_pingTask, 
        CONFIG_TASK_CORE_PINGER); 
    #endif // CONFIG_PINGER_TASK_STATIC_ALLOCATION
    if (_pingTask) {
      if (createSuspended) {
        rloga_i("Task [ %s ] has been successfully created", pingerTaskName);
        pingerTaskSuspend();
        return pingerEventHandlerRegister();
      } else {
        rloga_i("Task [ %s ] has been successfully started", pingerTaskName);
        return true;
      };
    }
    else {
      rloga_e("Failed to create task for Internet checking!");
      eventLoopPostError(RE_SYS_ERROR, ESP_FAIL);
      return false;
    };
  };
  return false;
}

bool pingerTaskSuspend()
{
  if ((_pingTask) && (eTaskGetState(_pingTask) != eSuspended)) {
    vTaskSuspend(_pingTask);
    if (eTaskGetState(_pingTask) == eSuspended) {
      rloga_d("Task [ %s ] has been suspended", pingerTaskName);
      return true;
    } else {
      rloga_e("Failed to suspend task [ %s ]", pingerTaskName);
    };
  };
  return false;
}

bool pingerTaskResume()
{
  if ((_pingTask) && (eTaskGetState(_pingTask) == eSuspended)) {
    vTaskResume(_pingTask);
    if (eTaskGetState(_pingTask) != eSuspended) {
      rloga_i("Task [ %s ] has been successfully resumed", pingerTaskName);
      return true;
    } else {
      rloga_e("Failed to resume task [ %s ]", pingerTaskName);
    };
  };
  return false;
}

bool pingerTaskDelete()
{
  if (_pingTask) {
    vTaskDelete(_pingTask);
    _pingTask = nullptr;
    rloga_d("Task [ %s ] was deleted", pingerTaskName);
  };
  return true;
}

// -----------------------------------------------------------------------------------------------------------------------
// -------------------------------------------------- WiFi event handler -------------------------------------------------
// -----------------------------------------------------------------------------------------------------------------------

static void pingerWifiEventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
  // STA connected & got IP
  if (event_id == RE_WIFI_STA_GOT_IP) {
    if (_pingTask) {
      pingerTaskResume();
    } else {
      pingerTaskCreate(false);
    }
    if (_pingTask) {
      xTaskNotify(_pingTask, PING_START, eSetBits);
    };
  }

  // STA disconnected
  else if ((event_id == RE_WIFI_STA_DISCONNECTED) || (event_id == RE_WIFI_STA_STOPPED)) {
    if ((_pingTask) && (eTaskGetState(_pingTask) != eSuspended)) {
      xTaskNotify(_pingTask, PING_STOP, eSetBits);
    };
  }
}

static void pingerOtaEventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
  if ((event_id == RE_SYS_OTA) && (event_data)) {
    re_system_event_data_t* data = (re_system_event_data_t*)event_data;
    if (data->type == RE_SYS_SET) {
      pingerTaskSuspend();
    } else {
      pingerTaskResume();
    };
  };
}

bool pingerEventHandlerRegister()
{
  rlog_d(logTAG, "Register pinger event handlers...");
  bool ret = eventHandlerRegister(RE_WIFI_EVENTS, ESP_EVENT_ANY_ID, &pingerWifiEventHandler, nullptr);
  ret = ret && eventHandlerRegister(RE_SYSTEM_EVENTS, RE_SYS_OTA, &pingerOtaEventHandler, nullptr);
  #if CONFIG_MQTT_PINGER_ENABLE
    ret = ret && pingerMqttRegister();
  #endif // CONFIG_MQTT_PINGER_ENABLE
  return ret;
}
