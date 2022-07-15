#include "project_config.h"
#include "def_consts.h"
#include "stdint.h"
#if CONFIG_OPENMON_PINGER_RSSI
#include "reWiFi.h"
#endif // CONFIG_OPENMON_PINGER_RSSI

#if CONFIG_OPENMON_ENABLE && CONFIG_OPENMON_PINGER_ENABLE

#include "rePingerOM.h"

void pingerOpenMonInit()
{
  dsChannelInit(EDS_OPENMON, CONFIG_OPENMON_PINGER_ID, CONFIG_OPENMON_PINGER_TOKEN, CONFIG_OPENMON_MIN_INTERVAL, CONFIG_OPENMON_ERROR_INTERVAL);
}

void pingerOpenMonPublish(ping_publish_data_t* data)
{
  uint8_t omIndex = 1;
  char* omValues = nullptr;

  // Append RSSI
  #if CONFIG_OPENMON_PINGER_RSSI
    {
      wifi_ap_record_t wifi_info = wifiInfo();
      omValues = concat_strings_div(omValues, 
        malloc_stringf("p%d=%d", 
          omIndex, wifi_info.rssi),
        "&");
      omIndex++;
    }
  #endif // CONFIG_OPENMON_PINGER_RSSI

  // Append free HEAP
  #if CONFIG_OPENMON_PINGER_HEAP_FREE
    {
      double heap_total = (double)heap_caps_get_total_size(MALLOC_CAP_DEFAULT) / 1024.0;
      double heap_free = (double)heap_caps_get_free_size(MALLOC_CAP_DEFAULT) / 1024.0;
      omValues = concat_strings_div(omValues, 
        malloc_stringf("p%d=%f&p%d=%f", 
          omIndex, heap_free, 
          omIndex+1, 100.0*heap_free/heap_total),
        "&");
      omIndex = omIndex+2;
    }
  #endif // CONFIG_OPENMON_PINGER_HEAP_FREE

  // Append hosts
  #if CONFIG_OPENMON_PINGER_HOSTS
    #ifdef CONFIG_PINGER_HOST_1
      omValues = concat_strings_div(omValues, 
        malloc_stringf("p%d=%d&p%d=%d&p%d=%f", 
          omIndex, data->host1.state, 
          omIndex+1, data->host1.duration_ms, 
          omIndex+2, data->host1.loss),
        "&");
      omIndex = omIndex+3;
    #endif // CONFIG_PINGER_HOST_1
    #ifdef CONFIG_PINGER_HOST_2
      omValues = concat_strings_div(omValues, 
        malloc_stringf("p%d=%d&p%d=%d&p%d=%f", 
          omIndex, data->host2.state, 
          omIndex+1, data->host2.duration_ms, 
          omIndex+2, data->host2.loss),
        "&");
      omIndex = omIndex+3;
    #endif // CONFIG_PINGER_HOST_2
    #ifdef CONFIG_PINGER_HOST_3
      omValues = concat_strings_div(omValues, 
        malloc_stringf("p%d=%d&p%d=%d&p%d=%f", 
          omIndex, data->host3.state, 
          omIndex+1, data->host3.duration_ms, 
          omIndex+2, data->host3.loss),
        "&");
      omIndex = omIndex+3;
    #endif // CONFIG_PINGER_HOST_3
  #endif // CONFIG_OPENMON_PINGER_HOSTS

  // Append internet ping
  omValues = concat_strings_div(omValues, 
    malloc_stringf("p%d=%d&p%d=%d&p%d=%d&p%d=%d&p%d=%f&p%d=%f&p%d=%f", 
      omIndex, data->inet.state, 
      omIndex+1, data->inet.duration_ms_total, 
      omIndex+2, data->inet.duration_ms_min,
      omIndex+3, data->inet.duration_ms_max, 
      omIndex+4, data->inet.loss_total, 
      omIndex+5, data->inet.loss_min, 
      omIndex+6, data->inet.loss_max),
    "&");
  
  // Send to queue
  if (omValues) {
    dsSend(EDS_OPENMON, CONFIG_OPENMON_PINGER_ID, omValues, false);
    free(omValues);
  };
}

#endif // CONFIG_OPENMON_ENABLE && CONFIG_OPENMON_PINGER_ENABLE