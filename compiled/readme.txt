StrongDC++ sqlite (c) pavel.pimenov@gmail.com

Домашняя страничка:
  http://flylinkdc.blogspot.com/

Скачать:
  http://www.flylinkdc.ru

Исходники (svn)
  Для чтения:
  http://flylinkdc.googlecode.com/svn/branches/strongdc-sqlite/
  Для разработчиков (требуется авторизация)
  https://flylinkdc.googlecode.com/svn/branches/strongdc-sqlite/

* Добавлен хаб-лист http://dchublist.ru/hublist.xml.bz2
* Пытаемся не падать под Wine (Linux)
* СтатистикаUpload/download по каждому юзеру
* Колонка Count IP и Last IP
* Из шары исключаются файлы с расширениям временных файлов (*.jc!,*.dmf,*.!ut,*.bc!,*.ob!,*.GetRight)
* Команда "Открыть свой файл-лист (Open own list)" вынесена в панель инструментов
* Встроенный фаервол (поддерживается IPTrust.ini и IPGuard)
* Запрещает шарить "Program Files" исключается утечка приватной информации: 
  - регистрационные ключи установленного ПО.
  - БД ICQ
* Убрал колонку Ratio в окне отдачи.
* Пытается блокировать "дубль-качков" (клиенты пытающиеся качать с разных хабов - 
   идентификация "качка" осуществляется по IP и размеру шары)
* Оптимизация в Identity 
* Добавлена визуализация статуса файлов в чужих файл-листах и окне поиска
   - Локальное расположение файла в своей шаре;
   - Файл уже качал;
   - Этот файл был у меня в шаре...
* SQLite 
* Вывод в чате коротких магнит-ссылок 
  (http://flylinkdc.blogspot.com/2008/04/stringdc-212-sqlite-r324.html#links)
* Поддержка рейтинга шары 
  (http://flylinkdc.blogspot.com/2008/04/flylinkdc-r321.html#links)
* В файл-лист добавлена дата и время хеширования (помогает найти "свежие" файлы)
* В файл-лист добавлен вывод bitrate(качество звука) для аудио-файлов *.mp3 
* Хранение TTH перенесено в базу данных SQLite
* Новая группа поиска "CD-DVD Image" 
* К типу поиска "аудио" добавлены: *.lqt .vqf
* К типу поиска "документ" добавлены: *.rtf,*.xls,*.odt,*.ods,*.odf,*.odp 
* Вырезаны настройки:
  - "Automatically follow redirects"