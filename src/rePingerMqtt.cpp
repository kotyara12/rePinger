#include <stdlib.h>
#include <stdbool.h>
#include "project_config.h"
#include "rLog.h"
#include "rePingerMqtt.h"
#include "rStrings.h"
#include "reMqtt.h"

static const char *tagPMQTT = "PING";
static char* _mqttTopicPing = nullptr;

char* mqttTopicPingerCreate(const bool primary)
{
  if (_mqttTopicPing) free(_mqttTopicPing);
  _mqttTopicPing = mqttGetTopic1(primary, CONFIG_MQTT_PINGER_LOCAL, CONFIG_MQTT_PINGER_TOPIC);
  rlog_i(tagPMQTT, "Generated topic for publishing ping result: [ %s ]", _mqttTopicPing);
  return _mqttTopicPing;
}

char* mqttTopicPingerGet()
{
  return _mqttTopicPing;
}

void mqttTopicPingerFree()
{
  if (_mqttTopicPing) free(_mqttTopicPing);
  _mqttTopicPing = nullptr;
  rlog_d(tagPMQTT, "Topic for publishing ping result has been scrapped");
}

#if CONFIG_MQTT_PINGER_AS_PLAIN

void pingerMqttPublishHostPlain(const char* topic, ping_host_data_t* data)
{
  char* _mqttTopicPingHost = mqttGetSubTopic(_mqttTopicPing, topic);
  if (_mqttTopicPingHost) {
    mqttPublish(mqttGetSubTopic(_mqttTopicPingHost, "hostname"), 
      (char*)data->host_name, 
      CONFIG_MQTT_PINGER_QOS, CONFIG_MQTT_PINGER_RETAINED, false, true, false);
    mqttPublish(mqttGetSubTopic(_mqttTopicPingHost, "state"), 
      malloc_stringf("%d", data->state), 
      CONFIG_MQTT_PINGER_QOS, CONFIG_MQTT_PINGER_RETAINED, false, true, true);
    mqttPublish(mqttGetSubTopic(_mqttTopicPingHost, "packets/transmitted"), 
      malloc_stringf("%d", data->transmitted), 
      CONFIG_MQTT_PINGER_QOS, CONFIG_MQTT_PINGER_RETAINED, false, true, true);
    mqttPublish(mqttGetSubTopic(_mqttTopicPingHost, "packets/received"), 
      malloc_stringf("%d", data->received), 
      CONFIG_MQTT_PINGER_QOS, CONFIG_MQTT_PINGER_RETAINED, false, true, true);
    mqttPublish(mqttGetSubTopic(_mqttTopicPingHost, "packets/ttl"), 
      malloc_stringf("%d", data->ttl), 
      CONFIG_MQTT_PINGER_QOS, CONFIG_MQTT_PINGER_RETAINED, false, true, true);

    mqttPublish(mqttGetSubTopic(_mqttTopicPingHost, "duration/" CONFIG_SENSOR_NUMERIC_VALUE), 
      malloc_stringf(CONFIG_FORMAT_PING_TIMERESP_VALUE, data->duration_ms), 
      CONFIG_MQTT_PINGER_QOS, CONFIG_MQTT_PINGER_RETAINED, false, true, true);
    mqttPublish(mqttGetSubTopic(_mqttTopicPingHost, "duration/" CONFIG_SENSOR_STRING_VALUE), 
      malloc_stringf(CONFIG_FORMAT_PING_TIMERESP_STRING, data->duration_ms), 
      CONFIG_MQTT_PINGER_QOS, CONFIG_MQTT_PINGER_RETAINED, false, true, true);
      
    mqttPublish(mqttGetSubTopic(_mqttTopicPingHost, "loss/" CONFIG_SENSOR_NUMERIC_VALUE), 
      malloc_stringf(CONFIG_FORMAT_PING_LOSS_VALUE, data->loss), 
      CONFIG_MQTT_PINGER_QOS, CONFIG_MQTT_PINGER_RETAINED, false, true, true);
    mqttPublish(mqttGetSubTopic(_mqttTopicPingHost, "loss/" CONFIG_SENSOR_STRING_VALUE), 
      malloc_stringf(CONFIG_FORMAT_PING_LOSS_STRING, data->loss), 
      CONFIG_MQTT_PINGER_QOS, CONFIG_MQTT_PINGER_RETAINED, false, true, true);

    mqttPublish(mqttGetSubTopic(_mqttTopicPingHost, "unavailable/time/unix"), 
      malloc_stringf("%d", data->time_unavailable), 
      CONFIG_MQTT_PINGER_QOS, CONFIG_MQTT_PINGER_RETAINED, false, true, true);
    mqttPublish(mqttGetSubTopic(_mqttTopicPingHost, "unavailable/time/string"), 
      malloc_timestr(CONFIG_FORMAT_DTS, data->time_unavailable), 
      CONFIG_MQTT_PINGER_QOS, CONFIG_MQTT_PINGER_RETAINED, false, true, true);

    free(_mqttTopicPingHost);
  };
}

void pingerMqttPublishInetPlain(ping_inet_data_t* data)
{
  mqttPublish(mqttGetSubTopic(_mqttTopicPing, "internet/state"), 
    malloc_stringf("%d", data->state), 
    CONFIG_MQTT_PINGER_QOS, CONFIG_MQTT_PINGER_RETAINED, false, true, true);

  mqttPublish(mqttGetSubTopic(_mqttTopicPing, "internet/duration/min/" CONFIG_SENSOR_NUMERIC_VALUE), 
    malloc_stringf(CONFIG_FORMAT_PING_TIMERESP_VALUE, data->duration_ms_min), 
    CONFIG_MQTT_PINGER_QOS, CONFIG_MQTT_PINGER_RETAINED, false, true, true);
  mqttPublish(mqttGetSubTopic(_mqttTopicPing, "internet/duration/min/" CONFIG_SENSOR_STRING_VALUE), 
    malloc_stringf(CONFIG_FORMAT_PING_TIMERESP_STRING, data->duration_ms_min), 
    CONFIG_MQTT_PINGER_QOS, CONFIG_MQTT_PINGER_RETAINED, false, true, true);

  mqttPublish(mqttGetSubTopic(_mqttTopicPing, "internet/duration/avg/" CONFIG_SENSOR_NUMERIC_VALUE), 
    malloc_stringf(CONFIG_FORMAT_PING_TIMERESP_VALUE, data->duration_ms_avg), 
    CONFIG_MQTT_PINGER_QOS, CONFIG_MQTT_PINGER_RETAINED, false, true, true);
  mqttPublish(mqttGetSubTopic(_mqttTopicPing, "internet/duration/avg/" CONFIG_SENSOR_STRING_VALUE), 
    malloc_stringf(CONFIG_FORMAT_PING_TIMERESP_STRING, data->duration_ms_avg), 
    CONFIG_MQTT_PINGER_QOS, CONFIG_MQTT_PINGER_RETAINED, false, true, true);

  mqttPublish(mqttGetSubTopic(_mqttTopicPing, "internet/duration/max/" CONFIG_SENSOR_NUMERIC_VALUE), 
    malloc_stringf(CONFIG_FORMAT_PING_TIMERESP_VALUE, data->duration_ms_max), 
    CONFIG_MQTT_PINGER_QOS, CONFIG_MQTT_PINGER_RETAINED, false, true, true);
  mqttPublish(mqttGetSubTopic(_mqttTopicPing, "internet/duration/max/" CONFIG_SENSOR_STRING_VALUE), 
    malloc_stringf(CONFIG_FORMAT_PING_TIMERESP_STRING, data->duration_ms_max), 
    CONFIG_MQTT_PINGER_QOS, CONFIG_MQTT_PINGER_RETAINED, false, true, true);

  mqttPublish(mqttGetSubTopic(_mqttTopicPing, "internet/loss/min/" CONFIG_SENSOR_NUMERIC_VALUE), 
    malloc_stringf(CONFIG_FORMAT_PING_LOSS_VALUE, data->loss_min), 
    CONFIG_MQTT_PINGER_QOS, CONFIG_MQTT_PINGER_RETAINED, false, true, true);
  mqttPublish(mqttGetSubTopic(_mqttTopicPing, "internet/loss/min/" CONFIG_SENSOR_STRING_VALUE), 
    malloc_stringf(CONFIG_FORMAT_PING_LOSS_STRING, data->loss_min), 
    CONFIG_MQTT_PINGER_QOS, CONFIG_MQTT_PINGER_RETAINED, false, true, true);

  mqttPublish(mqttGetSubTopic(_mqttTopicPing, "internet/loss/max/" CONFIG_SENSOR_NUMERIC_VALUE), 
    malloc_stringf(CONFIG_FORMAT_PING_LOSS_VALUE, data->loss_max), 
    CONFIG_MQTT_PINGER_QOS, CONFIG_MQTT_PINGER_RETAINED, false, true, true);
  mqttPublish(mqttGetSubTopic(_mqttTopicPing, "internet/loss/max/" CONFIG_SENSOR_STRING_VALUE), 
    malloc_stringf(CONFIG_FORMAT_PING_LOSS_STRING, data->loss_max), 
    CONFIG_MQTT_PINGER_QOS, CONFIG_MQTT_PINGER_RETAINED, false, true, true);

  mqttPublish(mqttGetSubTopic(_mqttTopicPing, "internet/unavailable/time/unix"), 
    malloc_stringf("%d", data->time_unavailable), 
    CONFIG_MQTT_PINGER_QOS, CONFIG_MQTT_PINGER_RETAINED, false, true, true);
  mqttPublish(mqttGetSubTopic(_mqttTopicPing, "internet/unavailable/time/string"), 
    malloc_timestr(CONFIG_FORMAT_DTS, data->time_unavailable), 
    CONFIG_MQTT_PINGER_QOS, CONFIG_MQTT_PINGER_RETAINED, false, true, true);

  mqttPublish(mqttGetSubTopic(_mqttTopicPing, "internet/unavailable/count"), 
    malloc_stringf("%d", data->count_unavailable), 
    CONFIG_MQTT_PINGER_QOS, CONFIG_MQTT_PINGER_RETAINED, false, true, true);

  #ifdef CONFIG_FORMAT_PING_MIXED
    switch (data->state) {
      case PING_AVAILABLE:
        mqttPublish(mqttGetSubTopic(_mqttTopicPing, "internet/" CONFIG_SENSOR_DISPLAY), 
          malloc_stringf(CONFIG_FORMAT_PING_MIXED, CONFIG_FORMAT_PING_OK, data->duration_ms_min, data->loss_min), 
          CONFIG_MQTT_PINGER_QOS, CONFIG_MQTT_PINGER_RETAINED, false, true, true);
        break;
      case PING_BAD:
        mqttPublish(mqttGetSubTopic(_mqttTopicPing, "internet/" CONFIG_SENSOR_DISPLAY), 
          malloc_stringf(CONFIG_FORMAT_PING_MIXED, CONFIG_FORMAT_PING_BAD, data->duration_ms_min, data->loss_min), 
          CONFIG_MQTT_PINGER_QOS, CONFIG_MQTT_PINGER_RETAINED, false, true, true);
        break;
      default:
        mqttPublish(mqttGetSubTopic(_mqttTopicPing, "internet/" CONFIG_SENSOR_DISPLAY), 
          malloc_stringf(CONFIG_FORMAT_PING_MIXED, CONFIG_FORMAT_PING_UNAVAILABLED, data->duration_ms_min, data->loss_min), 
          CONFIG_MQTT_PINGER_QOS, CONFIG_MQTT_PINGER_RETAINED, false, true, true);
        break;
    }
  #endif // CONFIG_FORMAT_PING_MIXED
}
#endif // CONFIG_MQTT_PINGER_AS_PLAIN

#if CONFIG_MQTT_PINGER_AS_JSON

char* pingerMqttPublishHostJson(ping_host_data_t* data)
{
  char* json_host = nullptr;

  char* json_packets = malloc_stringf("\"packets\":{\"transmitted\":%d,\"received\":%d,\"ttl\":%d}", 
    data->transmitted, data->received, data->ttl);
  char* json_duration = malloc_stringf("\"duration\":{\"" CONFIG_SENSOR_NUMERIC_VALUE "\":" CONFIG_FORMAT_PING_TIMERESP_VALUE ",\"" CONFIG_SENSOR_STRING_VALUE "\":\"" CONFIG_FORMAT_PING_TIMERESP_STRING "\"}", 
    data->duration_ms, data->duration_ms);
  char* json_loss = malloc_stringf("\"loss\":{\"" CONFIG_SENSOR_NUMERIC_VALUE "\":" CONFIG_FORMAT_PING_LOSS_VALUE ",\"" CONFIG_SENSOR_STRING_VALUE "\":\"" CONFIG_FORMAT_PING_LOSS_STRING "\"}", 
    data->loss, data->loss);
  char* json_unavailable = nullptr;
    if (data->state > PING_BAD) {
    char* s_unavailable = malloc_timestr(CONFIG_FORMAT_DTS, data->time_unavailable);
    if (s_unavailable) {
      json_unavailable = malloc_stringf("\"unavailable\":{\"time\":{\"unix\":%d,\"string\":\"%s\"}}", 
        data->time_unavailable, s_unavailable);
      free(s_unavailable);
    };
  };
  
  if ((json_packets) && (json_duration) && (json_loss)) {
    if (json_unavailable) {
      json_host = malloc_stringf("{\"hostname\":\"%s\",\"state\":%d,%s,%s,%s,%s}",
        data->host_name, data->state, json_packets, json_duration, json_loss, json_unavailable);
    } else {
      json_host = malloc_stringf("{\"hostname\":\"%s\",\"state\":%d,%s,%s,%s}",
        data->host_name, data->state, json_packets, json_duration, json_loss);
    };
  };
  
  if (json_packets) free(json_packets);
  if (json_duration) free(json_duration);
  if (json_loss) free(json_loss);
  if (json_unavailable) free(json_unavailable);
  
  return json_host;
}

char* pingerMqttPublishInetJson(ping_inet_data_t* data)
{
  char* json_inet = nullptr;

  char* json_duration = nullptr;
  char* json_duration_min = malloc_stringf("\"min\":{\"" CONFIG_SENSOR_NUMERIC_VALUE "\":" CONFIG_FORMAT_PING_TIMERESP_VALUE ",\"" CONFIG_SENSOR_STRING_VALUE "\":\"" CONFIG_FORMAT_PING_TIMERESP_STRING "\"}", 
    data->duration_ms_min, data->duration_ms_min);
  char* json_duration_avg = malloc_stringf("\"avg\":{\"" CONFIG_SENSOR_NUMERIC_VALUE "\":" CONFIG_FORMAT_PING_TIMERESP_VALUE ",\"" CONFIG_SENSOR_STRING_VALUE "\":\"" CONFIG_FORMAT_PING_TIMERESP_STRING "\"}", 
    data->duration_ms_avg, data->duration_ms_avg);
  char* json_duration_max = malloc_stringf("\"max\":{\"" CONFIG_SENSOR_NUMERIC_VALUE "\":" CONFIG_FORMAT_PING_TIMERESP_VALUE ",\"" CONFIG_SENSOR_STRING_VALUE "\":\"" CONFIG_FORMAT_PING_TIMERESP_STRING "\"}", 
    data->duration_ms_max, data->duration_ms_max);
  if ((json_duration_min) && (json_duration_avg) && (json_duration_max)) {
    json_duration = malloc_stringf("\"duration\":{%s,%s,%s}", 
      json_duration_min, json_duration_avg, json_duration_max);
  };
  if (json_duration_min) free(json_duration_min);
  if (json_duration_avg) free(json_duration_avg);
  if (json_duration_max) free(json_duration_max);

  char* json_loss = nullptr;
  char* json_loss_min = malloc_stringf("\"min\":{\"" CONFIG_SENSOR_NUMERIC_VALUE "\":" CONFIG_FORMAT_PING_LOSS_VALUE ",\"" CONFIG_SENSOR_STRING_VALUE "\":\"" CONFIG_FORMAT_PING_LOSS_STRING "\"}", 
    data->loss_min, data->loss_min);
  char* json_loss_max = malloc_stringf("\"max\":{\"" CONFIG_SENSOR_NUMERIC_VALUE "\":" CONFIG_FORMAT_PING_LOSS_VALUE ",\"" CONFIG_SENSOR_STRING_VALUE "\":\"" CONFIG_FORMAT_PING_LOSS_STRING "\"}", 
    data->loss_max, data->loss_max);
  if ((json_loss_min) && (json_loss_max)) {
    json_loss = malloc_stringf("\"loss\":{%s,%s}", 
      json_loss_min, json_loss_max);
  };
  if (json_loss_min) free(json_loss_min);
  if (json_loss_max) free(json_loss_max);

  char* json_unavailable = nullptr;
  if (data->state > PING_BAD) {
    char* s_unavailable = malloc_timestr(CONFIG_FORMAT_DTS, data->time_unavailable);
    if (s_unavailable) {
      json_unavailable = malloc_stringf("\"unavailable\":{\"time\":{\"unix\":%d,\"string\":\"%s\"},\"count\":%d}", 
        data->time_unavailable, s_unavailable, data->count_unavailable);
      free(s_unavailable);
    };
  };

  #ifdef CONFIG_FORMAT_PING_MIXED
    switch (data->state) {
      case PING_AVAILABLE:
        if ((json_duration) && (json_loss)) {
          json_inet = malloc_stringf("{\"state\":%d,%s,%s,\"%s\":\"" CONFIG_FORMAT_PING_MIXED "\"}",
            data->state, json_duration, json_loss, CONFIG_SENSOR_DISPLAY, CONFIG_FORMAT_PING_OK, data->duration_ms_min, data->loss_min);
        };
        break;
      case PING_BAD:
        if ((json_duration) && (json_loss)) {
          json_inet = malloc_stringf("{\"state\":%d,%s,%s,\"%s\":\"" CONFIG_FORMAT_PING_MIXED "\"}",
            data->state, json_duration, json_loss, CONFIG_SENSOR_DISPLAY, CONFIG_FORMAT_PING_BAD, data->duration_ms_min, data->loss_min);
        };
        break;
      default:
        if ((json_duration) && (json_loss) && (json_unavailable)) {
          json_inet = malloc_stringf("{\"state\":%d,%s,%s,%s,\"%s\":\"" CONFIG_FORMAT_PING_MIXED "\"}",
            data->state, json_duration, json_loss, json_unavailable, CONFIG_SENSOR_DISPLAY, CONFIG_FORMAT_PING_UNAVAILABLED, data->duration_ms_min, data->loss_min);
        };
        break;
    }
  #else
    if ((json_duration) && (json_loss)) {
      if (json_unavailable) {
        json_inet = malloc_stringf("{\"state\":%d,%s,%s,%s}",
          data->state, json_duration, json_loss, json_unavailable);
      } else {
        json_inet = malloc_stringf("{\"state\":%d,%s,%s}",
          data->state, json_duration, json_loss);
      };
    };
  #endif // CONFIG_FORMAT_PING_MIXED

  if (json_duration) free(json_duration);
  if (json_loss) free(json_loss);
  if (json_unavailable) free(json_unavailable);

  return json_inet;
}

#endif // CONFIG_MQTT_PINGER_AS_JSON

void pingerMqttPublish(ping_publish_data_t* data)
{
  if ((_mqttTopicPing) && mqttIsConnected()) {
    #if CONFIG_MQTT_PINGER_AS_PLAIN
      pingerMqttPublishHostPlain("host1", &data->host1);
      #ifdef CONFIG_PINGER_HOST_2
        pingerMqttPublishHostPlain("host2", &data->host2);
      #endif // CONFIG_PINGER_HOST_2
      #ifdef CONFIG_PINGER_HOST_3
        pingerMqttPublishHostPlain("host3", &data->host3);
      #endif // CONFIG_PINGER_HOST_3
      pingerMqttPublishInetPlain(&data->inet);
    #endif // CONFIG_MQTT_PINGER_AS_PLAIN

    #if CONFIG_MQTT_PINGER_AS_JSON
      char* json_host1 = nullptr;
      char* json_host2 = nullptr;
      char* json_host3 = nullptr;
      char* json_inet = nullptr;
      char* json_full = nullptr;
      json_host1 = pingerMqttPublishHostJson(&data->host1);
      #ifdef CONFIG_PINGER_HOST_2
        json_host2 = pingerMqttPublishHostJson(&data->host2);
      #endif // CONFIG_PINGER_HOST_2
      #ifdef CONFIG_PINGER_HOST_3
        json_host3 = pingerMqttPublishHostJson(&data->host3);
      #endif // CONFIG_PINGER_HOST_3
      json_inet = pingerMqttPublishInetJson(&data->inet);
      if (json_inet) {
        if (json_host1) {
          if (json_host2) {
            if (json_host3) {
              json_full = malloc_stringf("{\"internet\":%s,\"host1\":%s,\"host2\":%s,\"host3\":%s}", json_inet, json_host1, json_host2, json_host3);
              free(json_host3);
            } else {
              json_full = malloc_stringf("{\"internet\":%s,\"host1\":%s,\"host2\":%s}", json_inet, json_host1, json_host2);
            };
            free(json_host2);
          } else {
            json_full = malloc_stringf("{\"internet\":%s,\"host1\":%s}", json_inet, json_host1);
          };
          free(json_host1);
        } else {
          json_full = malloc_stringf("{%s}", json_inet);
        };
        free(json_inet);
        if (json_full) {
          mqttPublish(_mqttTopicPing, json_full, 
            CONFIG_MQTT_PINGER_QOS, CONFIG_MQTT_PINGER_RETAINED, false, false, true);
        };
      };
    #endif // CONFIG_MQTT_PINGER_AS_JSON
  };
}

// -----------------------------------------------------------------------------------------------------------------------
// -------------------------------------------------- WiFi event handler -------------------------------------------------
// -----------------------------------------------------------------------------------------------------------------------

static void pingerMqttEventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
  // MQTT connected
  if (event_id == RE_MQTT_CONNECTED) {
    re_mqtt_event_data_t* data = (re_mqtt_event_data_t*)event_data;
    mqttTopicPingerCreate(data->primary);
  } 
  // MQTT disconnected
  else if ((event_id == RE_MQTT_CONN_LOST) || (event_id == RE_MQTT_CONN_FAILED)) {
    mqttTopicPingerFree();
  };
}

bool pingerMqttRegister()
{
  return eventHandlerRegister(RE_MQTT_EVENTS, RE_MQTT_CONNECTED, &pingerMqttEventHandler, nullptr)
      && eventHandlerRegister(RE_MQTT_EVENTS, RE_MQTT_CONN_LOST, &pingerMqttEventHandler, nullptr);
};
