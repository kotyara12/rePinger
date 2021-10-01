/* 
   EN: Publishing server check results on MQTT broker
   RU: Публикация результатов проверки серверов на MQTT брокере
   --------------------------
   (с) 2021 Разживин Александр | Razzhivin Alexander
   kotyara12@yandex.ru | https://kotyara12.ru | tg: @kotyara1971
*/

#ifndef __RE_PINGERMQTT_H__
#define __RE_PINGERMQTT_H__

#include "reEvents.h"

#ifdef __cplusplus
extern "C" {
#endif

char* mqttTopicPingerCreate(const bool primary);
char* mqttTopicPingerGet();
void  mqttTopicPingerFree();

void pingerMqttPublish(ping_publish_data_t* data);

bool pingerMqttRegister();

#ifdef __cplusplus
}
#endif

#endif // __RE_PINGERMQTT_H__

