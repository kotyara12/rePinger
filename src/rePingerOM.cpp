#include "project_config.h"
#include "def_consts.h"

#if CONFIG_OPENMON_ENABLE && CONFIG_OPENMON_PINGER_ENABLE

#include <stdlib.h>
#include <stdbool.h>
#include "rLog.h"
#include "rePingerOM.h"
#include "rStrings.h"
#include "reOpenMon.h"
#if CONFIG_OPENMON_PINGER_RSSI
#include "reWiFi.h"
#endif // CONFIG_OPENMON_PINGER_RSSI

void pingerOpenMonInit()
{
  omControllerInit(CONFIG_OPENMON_PINGER_ID, CONFIG_OPENMON_PINGER_TOKEN, CONFIG_OPENMON_PINGER_INTERVAL);
}

void pingerOpenMonPublish(ping_publish_data_t* data)
{
  #if CONFIG_OPENMON_PINGER_RSSI
  wifi_ap_record_t wifi_info = wifiInfo();
  #endif // CONFIG_OPENMON_PINGER_RSSI

  #if CONFIG_OPENMON_PINGER_HEAP_FREE
  double heap_total = (double)heap_caps_get_total_size(MALLOC_CAP_DEFAULT) / 1024.0;
  double heap_free  = (double)heap_caps_get_free_size(MALLOC_CAP_DEFAULT) / 1024.0;
  #endif // CONFIG_OPENMON_PINGER_HEAP_FREE

  if (data->inet.state < PING_UNAVAILABLE) {
    #if CONFIG_OPENMON_PINGER_RSSI
      #if CONFIG_OPENMON_PINGER_HEAP_FREE
        #if CONFIG_OPENMON_PINGER_HOSTS
          
          // RSSI + HEAP + HOSTS + INET

          #ifdef CONFIG_PINGER_HOST_2
            #ifdef CONFIG_PINGER_HOST_3
              omSend(CONFIG_OPENMON_PINGER_ID, 
                malloc_stringf("p1=%d&p2=%f&p3=%f&p4=%d&p5=%d&p6=%f&p7=%d&p8=%d&p9=%f&p10=%d&p11=%d&p12=%f&p13=%d&p14=%d&p15=%d&p16=%d&p17=%f&p18=%f&p19=%f",
                  wifi_info.rssi,
                  heap_free, 100.0*heap_free/heap_total,
                  data->host1.state, data->host1.duration_ms, data->host1.loss,
                  data->host2.state, data->host2.duration_ms, data->host2.loss,
                  data->host3.state, data->host3.duration_ms, data->host3.loss,
                  data->inet.state, data->inet.duration_ms_total, data->inet.duration_ms_min, data->inet.duration_ms_max, data->inet.loss_total, data->inet.loss_min, data->inet.loss_max));
            #else
              omSend(CONFIG_OPENMON_PINGER_ID, 
                malloc_stringf("p1=%d&p2=%f&p3=%f&p4=%d&p5=%d&p6=%f&p7=%d&p8=%d&p9=%f&p10=%d&p11=%d&p12=%d&p13=%d&p14=%f&p15=%f&p16=%f",
                  wifi_info.rssi,
                  heap_free, 100.0*heap_free/heap_total,
                  data->host1.state, data->host1.duration_ms, data->host1.loss,
                  data->host2.state, data->host2.duration_ms, data->host2.loss,
                  data->inet.state, data->inet.duration_ms_total, data->inet.duration_ms_min, data->inet.duration_ms_max, data->inet.loss_total, data->inet.loss_min, data->inet.loss_max));
            #endif // CONFIG_PINGER_HOST_3
          #else
            omSend(CONFIG_OPENMON_PINGER_ID, 
              malloc_stringf("p1=%d&p2=%f&p3=%f&p4=%d&p5=%d&p6=%f&p7=%d&p8=%d&p9=%d&p10=%d&p11=%f&p12=%f&p13=%f",
                wifi_info.rssi,
                heap_free, 100.0*heap_free/heap_total,
                data->host1.state, data->host1.duration_ms, data->host1.loss,
                data->inet.state, data->inet.duration_ms_total, data->inet.duration_ms_min, data->inet.duration_ms_max, data->inet.loss_total, data->inet.loss_min, data->inet.loss_max));
          #endif // CONFIG_PINGER_HOST_2

        #else

          // RSSI + HEAP + INET

          omSend(CONFIG_OPENMON_PINGER_ID, 
            malloc_stringf("p1=%d&p2=%f&p3=%f&p4=%d&p5=%d&p6=%d&p7=%d&p8=%f&p9=%f&p10=%f",
              wifi_info.rssi,
              heap_free, 100.0*heap_free/heap_total,
              data->inet.state, data->inet.duration_ms_total, data->inet.duration_ms_min, data->inet.duration_ms_max, data->inet.loss_total, data->inet.loss_min, data->inet.loss_max));

        #endif // CONFIG_OPENMON_PINGER_HOSTS
      #else
        #if CONFIG_OPENMON_PINGER_HOSTS

          // RSSI + HOSTS + INET
        
          #ifdef CONFIG_PINGER_HOST_2
            #ifdef CONFIG_PINGER_HOST_3
              omSend(CONFIG_OPENMON_PINGER_ID, 
                malloc_stringf("p1=%d&p2=%d&p3=%d&p4=%f&p5=%d&p6=%d&p7=%f&p8=%d&p9=%d&p10=%f&p11=%d&p12=%d&p13=%d&p14=%d&p15=%f&p16=%f&p17=%f",
                  wifi_info.rssi,
                  data->host1.state, data->host1.duration_ms, data->host1.loss,
                  data->host2.state, data->host2.duration_ms, data->host2.loss,
                  data->host3.state, data->host3.duration_ms, data->host3.loss,
                  data->inet.state, data->inet.duration_ms_total, data->inet.duration_ms_min, data->inet.duration_ms_max, data->inet.loss_total, data->inet.loss_min, data->inet.loss_max));
            #else
              omSend(CONFIG_OPENMON_PINGER_ID, 
                malloc_stringf("p1=%d&p2=%d&p3=%d&p4=%f&p5=%d&p6=%d&p7=%f&p8=%d&p9=%d&p10=%d&p11=%d&p12=%f&p13=%f&p14=%f",
                  wifi_info.rssi,
                  data->host1.state, data->host1.duration_ms, data->host1.loss,
                  data->host2.state, data->host2.duration_ms, data->host2.loss,
                  data->inet.state, data->inet.duration_ms_total, data->inet.duration_ms_min, data->inet.duration_ms_max, data->inet.loss_total, data->inet.loss_min, data->inet.loss_max));
            #endif // CONFIG_PINGER_HOST_3
          #else
            omSend(CONFIG_OPENMON_PINGER_ID, 
              malloc_stringf("p1=%d&p2=%d&p3=%d&p4=%f&p5=%d&p6=%d&p7=%d&p8=%d&p9=%f&p10=%f&p11=%f",
                wifi_info.rssi,
                data->host1.state, data->host1.duration_ms, data->host1.loss,
                data->inet.state, data->inet.duration_ms_total, data->inet.duration_ms_min, data->inet.duration_ms_max, data->inet.loss_total, data->inet.loss_min, data->inet.loss_max));
          #endif // CONFIG_PINGER_HOST_2
        
        #else
        
          // RSSI + INET
        
          omSend(CONFIG_OPENMON_PINGER_ID, 
            malloc_stringf("p1=%d&p2=%d&p3=%d&p4=%d&p5=%d&p6=%f&p7=%f&p8=%f",
              wifi_info.rssi,
              data->inet.state, data->inet.duration_ms_total, data->inet.duration_ms_min, data->inet.duration_ms_max, data->inet.loss_total, data->inet.loss_min, data->inet.loss_max));

        #endif // CONFIG_OPENMON_PINGER_HOSTS
      #endif // CONFIG_OPENMON_PINGER_HEAP_FREE
    #else
      #if CONFIG_OPENMON_PINGER_HEAP_FREE
        #if CONFIG_OPENMON_PINGER_HOSTS

          // HEAP + HOSTS + INET
        
          #ifdef CONFIG_PINGER_HOST_2
            #ifdef CONFIG_PINGER_HOST_3
              omSend(CONFIG_OPENMON_PINGER_ID, 
                malloc_stringf("p1=%f&p2=%f&p3=%d&p4=%d&p5=%f&p6=%d&p7=%d&p8=%f&p9=%d&p10=%d&p11=%f&p12=%d&p13=%d&p14=%d&p15=%d&p16=%f&p17=%f&p18=%f",
                  heap_free, 100.0*heap_free/heap_total,
                  data->host1.state, data->host1.duration_ms, data->host1.loss,
                  data->host2.state, data->host2.duration_ms, data->host2.loss,
                  data->host3.state, data->host3.duration_ms, data->host3.loss,
                  data->inet.state, data->inet.duration_ms_total, data->inet.duration_ms_min, data->inet.duration_ms_max, data->inet.loss_total, data->inet.loss_min, data->inet.loss_max));
            #else
              omSend(CONFIG_OPENMON_PINGER_ID, 
                malloc_stringf("p1=%f&p2=%f&p3=%d&p4=%d&p5=%f&p6=%d&p7=%d&p8=%f&p9=%d&p10=%d&p11=%d&p12=%d&p13=%f&p14=%f&p15=%f",
                  heap_free, 100.0*heap_free/heap_total,
                  data->host1.state, data->host1.duration_ms, data->host1.loss,
                  data->host2.state, data->host2.duration_ms, data->host2.loss,
                  data->inet.state, data->inet.duration_ms_total, data->inet.duration_ms_min, data->inet.duration_ms_max, data->inet.loss_total, data->inet.loss_min, data->inet.loss_max));
            #endif // CONFIG_PINGER_HOST_3
          #else
            omSend(CONFIG_OPENMON_PINGER_ID, 
              malloc_stringf("p1=%f&p2=%f&p3=%d&p4=%d&p5=%f&p6=%d&p7=%d&p8=%d&p9=%d&p10=%f&p11=%f&p12=%f",
                heap_free, 100.0*heap_free/heap_total,
                data->host1.state, data->host1.duration_ms, data->host1.loss,
                data->inet.state, data->inet.duration_ms_total, data->inet.duration_ms_min, data->inet.duration_ms_max, data->inet.loss_total, data->inet.loss_min, data->inet.loss_max));
          #endif // CONFIG_PINGER_HOST_2

        #else
          
          // HEAP + INET
          
          omSend(CONFIG_OPENMON_PINGER_ID, 
            malloc_stringf("p1=%f&p2=%f&p3=%d&p4=%d&p5=%d&p6=%d&p7=%f&p8=%f&p9=%f",
              heap_free, 100.0*heap_free/heap_total,
              data->inet.state, data->inet.duration_ms_total, data->inet.duration_ms_min, data->inet.duration_ms_max, data->inet.loss_total, data->inet.loss_min, data->inet.loss_max));

        #endif // CONFIG_OPENMON_PINGER_HOSTS
      #else
        #if CONFIG_OPENMON_PINGER_HOSTS

          // HOSTS + INET

          #ifdef CONFIG_PINGER_HOST_2
            #ifdef CONFIG_PINGER_HOST_3
              omSend(CONFIG_OPENMON_PINGER_ID, 
                malloc_stringf("p1=%d&p2=%d&p3=%f&p4=%d&p5=%d&p6=%f&p7=%d&p8=%d&p9=%f&p10=%d&p11=%d&p12=%d&p13=%d&p14=%f&p15=%f&p16=%f",
                  data->host1.state, data->host1.duration_ms, data->host1.loss,
                  data->host2.state, data->host2.duration_ms, data->host2.loss,
                  data->host3.state, data->host3.duration_ms, data->host3.loss,
                  data->inet.state, data->inet.duration_ms_total, data->inet.duration_ms_min, data->inet.duration_ms_max, data->inet.loss_total, data->inet.loss_min, data->inet.loss_max));
            #else
              omSend(CONFIG_OPENMON_PINGER_ID, 
                malloc_stringf("p1=%d&p2=%d&p3=%f&p4=%d&p5=%d&p6=%f&p7=%d&p8=%d&p9=%d&p10=%d&p11=%f&p12=%f&p13=%f",
                  data->host1.state, data->host1.duration_ms, data->host1.loss,
                  data->host2.state, data->host2.duration_ms, data->host2.loss,
                  data->inet.state, data->inet.duration_ms_total, data->inet.duration_ms_min, data->inet.duration_ms_max, data->inet.loss_total, data->inet.loss_min, data->inet.loss_max));
            #endif // CONFIG_PINGER_HOST_3
          #else
            omSend(CONFIG_OPENMON_PINGER_ID, 
              malloc_stringf("p1=%d&p2=%d&p3=%f&p4=%d&p5=%d&p6=%d&p7=%d&p8=%f&p9=%f&p10=%f",
                data->host1.state, data->host1.duration_ms, data->host1.loss,
                data->inet.state, data->inet.duration_ms_total, data->inet.duration_ms_min, data->inet.duration_ms_max, data->inet.loss_total, data->inet.loss_min, data->inet.loss_max));
          #endif // CONFIG_PINGER_HOST_2
        #else

          // INET

          omSend(CONFIG_OPENMON_PINGER_ID, 
            malloc_stringf("p1=%d&p2=%d&p3=%d&p4=%d&p5=%f&p6=%f&p7=%f",
              data->inet.state, data->inet.duration_ms_total, data->inet.duration_ms_min, data->inet.duration_ms_max, data->inet.loss_total, data->inet.loss_min, data->inet.loss_max));

        #endif // CONFIG_OPENMON_PINGER_HOSTS
      #endif // CONFIG_OPENMON_PINGER_HEAP_FREE
    #endif // CONFIG_OPENMON_PINGER_RSSI
  };
}

#endif // CONFIG_OPENMON_ENABLE && CONFIG_OPENMON_PINGER_ENABLE