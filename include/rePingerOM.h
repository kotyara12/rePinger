/* 
   EN: Publishing server check results on open-monitoring.online
   RU: Публикация результатов проверки серверов на open-monitoring.online
   --------------------------
   (с) 2021 Разживин Александр | Razzhivin Alexander
   kotyara12@yandex.ru | https://kotyara12.ru | tg: @kotyara1971
*/

#ifndef __RE_PINGEROM_H__
#define __RE_PINGEROM_H__

#include <stdlib.h>
#include <stdbool.h>
#include "project_config.h"
#include "def_consts.h"
#include "rLog.h"
#include "rStrings.h"
#include "reEvents.h"
#include "reDataSend.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * ВАЖНО!
 * Контроллер должен быть определен следующим образом:
 * 
 * #if CONFIG_OPENMON_PINGER_RSSI
 * RSSI (целое)
 * #endif // CONFIG_OPENMON_PINGER_RSSI
 * 
 * #if CONFIG_OPENMON_PINGER_HEAP_FREE
 * Размер свободной кучи (число с запятой)
 * Размер свободной кучи в процентах (число с запятой)
 * #endif // CONFIG_OPENMON_PINGER_HEAP_FREE
 * 
 * #if CONFIG_OPENMON_PINGER_HOSTS
 * Статус хоста 1 (целое)
 * Задержка хоста 1 (целое)
 * Потери хоста 1 (число с запятой)
 * Статус хоста 2 (целое)           - если задан хост 2
 * Задержка хоста 2 (целое)         - если задан хост 2
 * Потери хоста 2 (число с запятой) - если задан хост 2
 * Статус хоста 3 (целое)           - если задан хост 3
 * Задержка хоста 3 (целое)         - если задан хост 3
 * Потери хоста 3 (число с запятой) - если задан хост 3
 * #endif // CONFIG_OPENMON_PINGER_HOSTS
 * 
 * Статус интернета (целое)
 * Итоговая задержка (фильтрованная) (целое)
 * Минимальная задержка (целое)
 * Максимальная задержка (целое)
 * Итоговые потери (число с запятой)
 * Минимальные потери (число с запятой)
 * Максимальные потери (число с запятой)
 * 
 * ! Данные будут отправлены на сервер именно в этом порядке !
 * 
 **/

void pingerOpenMonInit();
void pingerOpenMonPublish(ping_publish_data_t* data);

#ifdef __cplusplus
}
#endif

#endif // __RE_PINGEROM_H__

