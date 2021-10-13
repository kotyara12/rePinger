/* 
   EN: Service for periodically checking the availability of the Internet on one or several public servers
   RU: Служба периодической проверки доступности сети интернет по одному или нескольким публичным серверам
   --------------------------
   (с) 2021 Разживин Александр | Razzhivin Alexander
   kotyara12@yandex.ru | https://kotyara12.ru | tg: @kotyara1971
*/

#ifndef __RE_PINGER_H__
#define __RE_PINGER_H__ 

#include "time.h"
#include "project_config.h"
#include "def_consts.h"

#ifdef __cplusplus
extern "C" {
#endif

bool pingerTaskCreate(bool createSuspended);
bool pingerTaskSuspend();
bool pingerTaskResume();
bool pingerTaskDelete();

bool pingerEventHandlerRegister();

#ifdef __cplusplus
}
#endif

#endif // __RE_PINGER_H__