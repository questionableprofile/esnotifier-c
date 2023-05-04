# esnotifier-c

Сохраняет логи из БЛО.
Для сохранения логов в файл дополнительная конфигурация не требуется.

### Для сохранения логов в Telegram:
- Создать Telegram бота с помощью бота `@BotFather`
- Скопировать токен бота
- Создать файл config.txt в папке с программой
- Написать в нём `token=` и вставить токен
- Сохранить файл
- Написать Вашему боту `/start`

### Дополнительные опции:
- `owner` - айди аккаунта владельца (сохраняется автоматически после команды /start, если не установлено ранее)
- `port` - порт локального сервера

### Собственная сборка
#### Linux
- Установить [json-c](https://github.com/json-c/json-c) (скорее всего доступно в репозиториях дистрибутива)
- Установить [telebot](https://github.com/smartnode/telebot)
```
$ git clone https://github.com/questionableprofile/esnotifier-c
$ cd esnotifier-c
$ mkdir build && cd build
$ cmake ..
$ make
./notifier
```

#### Windows
- Пресс качат
- Бегит
- Турник
- Анжуманя
