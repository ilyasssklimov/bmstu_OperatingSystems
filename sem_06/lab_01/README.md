## Задание
Написанить собственного демона (kernel mode)

`Глава 13 - демоны`

## Защита
- Запустить демона
- Показать информацию о демоне ps -ajx (
  - parent id - почему такой, завершили процесс, поэтому демон осиротел;
  - в группе он один лидер группы, лидер сессии;
  - ? - нет управляющего терминала (отсутствует группа управляющего терминала);
  - почему три одинаковых идентификатора;
  - -1 - нет терминала и нет терминальной группы;
  - S - в прерываемом сне, s - процесс является лидером сессии, l - процесс является многпоточным)
- Точка в коде, где процесс становится демоном - deamonize
- Обеспечить запуск только в одно экземпляре (already_running() ?)
- lock() - какой системный вызов (control), передается дескриптор файла, ключи (какие)
- Где запускается дополнительный поток? В main - чтобы процесс мог перехватывать сигнал sighup. 
- Какие действия связаны с сигналом sighup? Сначала игнорирование, а затем в main опять восстанавливается реакция на сигнал по умолчанию.

`Группа переднего плана - это чушь`