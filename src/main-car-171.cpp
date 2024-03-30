
// C:\Users\Acer\Documents\PlatformIO\Projects\veloCars_GyverHub\.pio\build\car

// FULL DEMO
#include <Arduino.h>
// #define ATOMIC_FS_UPDATE

#define LED_BOARD 2
#define MOT1_PIN 27
#define MOT2_PIN 26
#define PWMCHAN1 0
#define PWMCHAN2 1
#define LEDPWM 3
#define BUT 0
#define STARTSPEED 55
#define STARTTIME 500 // сколько милисекунд происходит страгивание
#define MINSPEED 35
#define ACCELERATION 12 // ускорение: 1...50;
#define RELE1 2
#define ON 1
#define OFF 0

#define CHECKMOTOR // в начале программы крутанет колесами туда сюда
// #define MOTORDEBUG // отладка движения мотора

#include <EncButton.h>
EncButton eb(21, 19, 18);

// const char *ssid = "esp32";
// const char *password = "123456789000";
// const char *ssid = "kuso4ek_raya";
// const char *password = "1234567812345678";
const char *ssid = "Showmix";
const char *password = "tol9igor";
#define STATIC_IP

#ifdef STATIC_IP
IPAddress staticIP(192, 168, 3, 171); // важно правильно указать третье число - подсеть, смотри пояснения выше
IPAddress gateway(192, 168, 3, 1);    // и тут изменить тройку на свою подсеть
IPAddress subnet(255, 255, 255, 0);
IPAddress dns1(192, 168, 10, 1); // изменить тройку на свою подсеть
IPAddress dns2(8, 8, 8, 8);
#endif

#include <AsyncUDP.h>
AsyncUDP udp;
const uint16_t port = 12171; // Определяем порт
IPAddress velo181ip(192, 168, 3, 181);

#include <Stamp.h>
#include <StampTicker.h>
Datime nowDatime;
// Stamp fixedTimeStamp;
StampTicker nowTimeStamp;
StampTicker UpTimeStamp;

#define GH_INCLUDE_PORTAL // +4% к памяти программы и локальный портал присутствует.
#include <GyverHub.h>
GyverHub hub;

#include <PairsFile.h>
PairsFile data(&GH_FS, "/data.dat", 3000);

String inp;
bool allowRunMoto = 1; // можно ли крутиться мотору
int speed = 0;         // скорость мотора
unsigned int
    minSpeed = MINSPEED,     // скорость на которой машина еще двигается
    startSpeed = STARTSPEED, // скорость страгивания
    recievedSpeed = 0,       // полученная от педатей скорость
    currSpeed = 0;           // текущая скорость
bool carRun = 0;             // флаг, движется ли машина
bool slowBrake = 1;          // плавный тормоз
byte car_proc = 0;           // автомат машинки
uint32_t
    ms = 0,
    carMs = 0,
    startTime = STARTTIME,
    incrSpeedStep = 100 / ACCELERATION;

void parseUdpMessage()
{
    if (udp.listen(port))
    {
        udp.onPacket([](AsyncUDPPacket packet)
                     {
        /*
        // хороший сайт для работы с библиотекой AsyncUdp
        // https://community.appinventor.mit.edu/t/esp32-with-udp-send-receive-text-chat-mobile-mobile-udp-testing-extension-udp-by-ullis-ulrich-bien/72664/2

        // пример парсинга единственного первого байта данных
        String state;                       // Объект для хранения состояния светодиода в строковом формате
        const uint8_t* msg = packet.data(); // Записываем адрес начала данных в памяти
        const size_t len = packet.length(); // Записываем размер данных

        // Если адрес данных не равен нулю и размер данных больше нуля...
        if (msg != NULL && len > 0) {
          if (msg[0] != 0) {                    // Если первый байт данных содержит 0x1
            state = "Включён"; // записываем строку в объект String
            ledBlink();
          } else if (msg[0] == 0) {
            state = "OFF";
          }
          // Отправляем Обратно данные клиенту
          //packet.printf("Светодиод %s", state.c_str());
        }
        */
#ifdef UDPDEBUG
                         Serial.print("UDP Packet Type: ");
                         Serial.print(packet.isBroadcast() ? "Broadcast" : packet.isMulticast() ? "Multicast"
                                                                                                : "Unicast");
                         Serial.print(", From: ");
                         Serial.print(packet.remoteIP());
                         Serial.print(":");
                         Serial.print(packet.remotePort());
                         Serial.print(", To: ");
                         Serial.print(packet.localIP());
                         Serial.print(":");
                         Serial.print(packet.localPort());
                         Serial.print(", Length: ");
                         Serial.print(packet.length());
                         Serial.print(", Data: ");
                         Serial.write(packet.data(), packet.length());
                         Serial.println();
#endif
                         /*
                         // мощное преобразование входящего сообщения в String
                         //взято отсюда https://stackoverflow.com/questions/58253174/saving-packet-data-to-string-with-asyncudp-on-esp32
                         //String  udpMessage = (char*)(packet.data());
                         */

                         char *tmpStr = (char *)malloc(packet.length() + 1);
                         memcpy(tmpStr, packet.data(), packet.length());
                         tmpStr[packet.length()] = '\0'; // ensure null termination
                         String udpMessage = String(tmpStr);
                         free(tmpStr); // we can clean it now
#ifdef UDPDEBUG
                         Serial.print("income udp String: ");
                         Serial.print(udpMessage);
#endif
                         // udpMessage - полученная стрингга
                         recievedSpeed = udpMessage.toInt();
                         // reply to the client
                         // packet.printf("Got %u bytes of data", packet.length());
                     });
    } // udp.listen()
} // parseUdpMessage()

// web билдер
bool dsbl = 0, nolbl = 0, notab = 0;
void build(gh::Builder &b)
{
    // =================== КОНТЕЙНЕРЫ ===================
    /* вот так строить чтобы не накосячить
      {
        gh::Row r(b, 2); // контейнер сам создастся здесь

        {
          gh::Col c(b, 2); // контейнер сам создастся здесь
           b.Button().size(1).label(F("Красненькая")).color(gh::Colors::Red);
          b.Button();
          b.Button();
        } // контейнер сам закроется здесь
        {
          gh::Col d(b, 2); // контейнер сам создастся здесь
          b.Button();
          b.Button();

        } // контейнер сам закроется здесь
      }   // контейнер сам закроется здесь

    */

    // {
    // когда было NTP юзал это
    //     gh::Row r2(b);
    //     b.DateTime_(F("datime"), &nowTimeStamp.unix).label(F("Времечко"));
    // }

    {
        gh::Row r3(b);
        b.Time_(F("uptime"), &UpTimeStamp.unix).label(F("Аптайм"));
    }

    // из теплицы, для примера
    //    {
    //      gh::Row r4(b);
    //      b.Label_(F("dht11T")).size(2).label(F("в комнате температура")).color(gh::Colors::Orange);
    //      b.Label_(F("dht11H")).size(2).label(F("Влажность")).color(gh::Colors::Orange);
    //    }
    //    {
    //      gh::Row r5(b);
    //      b.Label_(F("dht22T")).size(2).label(F("За окном температура")).color(gh::Colors::Aqua);
    //      b.Label_(F("dht22H")).size(2).label(F(" Влажность")).color(gh::Colors::Aqua);
    //    }

    // в самом низу сделаем заголовок, текст будем обновлять из loop() (см. ниже)
    // b.Title_(F("title")).label(F("обн. из loop"));
    // =================== ОБРАБОТКА КНОПОК ===================

    {
        // Скорость двигателя , управление отсюда
        gh::Row r(b);
        b.Gauge_("gas").size(2).range(1, 255, 1).unit(" вжух").color(0x71abea);
        // задать из интерфейса  скорость (кнопка)
        // if (b.Button().label("рандом скорость").size(1).click())
        //     hub.update("gas").value(random(50));
    }

    // =============== НЕ ОТОбражается интерфейс ===============
    {
        gh::Row r7(b);
        if (b.Switch(&slowBrake).label(F("плавный тормоз")).size(1).click())
        {
        }
        b.Input(&speed).label(F("Введи скорость ")).size(1);
        b.Label_(F("inputed")).label(F("обновляемые данные")).size(1).value("default"); // с указанием стандартного значения
        if (b.Switch(&allowRunMoto).label(F("активировать мотор")).size(1).click())
        {
            // Serial.println(sw);
        }
    }

    // ==================== ОБНОВЛЕНИЕ ====================

    // для отправки обновления нужно знать ИМЯ компонента. Его можно задать почти у всех виджетов
    // к функции добавляется подчёркивание, всё остальное - как раньше

    // =================== ИНФО О БИЛДЕ ===================

    // можно получить информацию о билде и клиенте для своих целей
    // поставь тут 1, чтобы включить вывод =)
    if (0)
    {
        // запрос информации о виджетах
        if (b.build.isUI())
        {
            Serial.println("=== UI BUILD ===");
        }

        // действие с виджетом
        if (b.build.isSet())
        {
            Serial.println("=== SET ===");
            Serial.print("name: ");
            Serial.println(b.build.name);
            Serial.print("value: ");
            Serial.println(b.build.value);
        }

        Serial.print("client from: ");
        Serial.println(gh::readConnection(b.build.client.connection()));
        Serial.print("ID: ");
        Serial.println(b.build.client.id);
        Serial.println();
    }
}

// поддержка wifi связи
void wifiSupport()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        // Подключаемся к Wi-Fi
        Serial.print("try conn to ");
        Serial.print(ssid);
        Serial.print(":");
        WiFi.mode(WIFI_STA);
#ifdef STATIC_IP
        if (WiFi.config(staticIP, gateway, subnet, dns1, dns2) == false)
        {
            Serial.println("wifi config failed.");
            return;
        }
#endif
        WiFi.begin(ssid, password);
        uint8_t trycon = 0;
        while (WiFi.status() != WL_CONNECTED)
        {
            if (trycon++ < 30)
                delay(500);
            else
            {
                Serial.print("no connection to Wifi. Esp restarts NOW!");
                delay(1000);
                ESP.restart();
            }
        }
        Serial.println("Connected. \nIP: ");

        // Выводим IP ESP32
        Serial.println(WiFi.localIP());
    }
    // или режим точки доступа
    // WiFi.mode(WIFI_AP);
    // WiFi.softAP("My Hub");
    // Serial.println(WiFi.softAPIP());    // по умолч. 192.168.4.1
} // wifiSupport()

// слушаем энкодер ( для отладки нужен был)
void listen_eb()
{
    if (eb.click())
    {
        Serial.println(allowRunMoto);
        allowRunMoto = !allowRunMoto;
    }
    if (eb.left())
    {
        if ((speed < 35) && (speed > -35))
            speed = -35;
        else if (speed > -254)
            speed--;
        Serial.print("Speed ");
        Serial.println(speed);
    }
    if (eb.right())
    {
        if ((speed > -35) && (speed < 35))
            speed = 35;
        else if (speed < 254)
            speed++;
        Serial.print("Speed ");
        Serial.println(speed);
    }
} // listen_eb()

// крутим мотором
void setSpeed(int spd)
{
    if (allowRunMoto)
    {
        digitalWrite(LED_BOARD, allowRunMoto);

#ifdef MOTORDEBUG
        Serial.print(" Speed = ");
        Serial.print(spd);
#endif
        byte pwm = spd;
        if (spd < 0)
        { // reverse speeds
            pwm = spd;
#ifdef MOTORDEBUG
            Serial.print("\t pwm = ");
            Serial.println(pwm);
#endif
            ledcWrite(PWMCHAN1, pwm);
            ledcWrite(PWMCHAN2, 255);
        }
        else
        { // stop or forward
            pwm = 255 - spd;
#ifdef MOTORDEBUG
            Serial.print("\t pwm = ");
            Serial.println(pwm);
#endif
            ledcWrite(PWMCHAN1, 255);
            ledcWrite(PWMCHAN2, pwm);
        }
    }
    else // не разрешено вращение
    {
        digitalWrite(LED_BOARD, allowRunMoto);
        ledcWrite(PWMCHAN1, 0);
        ledcWrite(PWMCHAN2, 0);
        // тормоз
        //    ledcWrite(PWMCHAN1, 255);
        //    ledcWrite(PWMCHAN2, 255);
    }
} // setSpeed()

void setup()
{
    Serial.begin(115200);

    wifiSupport();

    // Stamp devStamp(2024, 1, 1, 0, 0, 0);
    // nowTimeStamp.update(devStamp.unix); // присваиваем текущему времени штамп 1 января 2024
    // Serial.print("Stamp: ");
    // Serial.println(nowTimeStamp.toString());
    Stamp initStamp(0, 1, 1, 0, 0, 0);
    UpTimeStamp.update(initStamp.unix); // присваиваем аптайму штамп 1 января 0
    setStampZone(5);

    hub.config(F("MyDevices"), F("car171"), F("")); // 1 - префикс, для приложения, 2 - имя устройства
    hub.onBuild(build);

    // запуск!
    hub.begin();
    data.begin();
    // проверка записи в память
    data.set("key2", 3.14);
    data.set("key3", F("abcd"));

    pinMode(BUT, INPUT);
    pinMode(LED_BOARD, OUTPUT);

    // config PWM esp32
    ledcSetup(PWMCHAN1, 5000, 8);      // chanel, freq, res
    ledcSetup(PWMCHAN2, 5000, 8);      // chanel, freq, res
    ledcAttachPin(MOT1_PIN, PWMCHAN1); // Pin, hannel
    ledcAttachPin(MOT2_PIN, PWMCHAN2); // Pin, Channel
#ifdef CHECKMOTOR
    // пробный прокрут колесами
    ledcWrite(PWMCHAN1, 180);
    ledcWrite(PWMCHAN2, 0);
    delay(500);
    ledcWrite(PWMCHAN1, 0);
    ledcWrite(PWMCHAN2, 180);
    delay(500);
#endif
    ledcWrite(PWMCHAN1, 0); // 0..255
    ledcWrite(PWMCHAN2, 0); // 0..255
} // setup

void loop()
{
    ms = millis();
    // =================== ТИКЕР ===================
    // вызываем тикер в главном цикле программы
    // он обеспечивает работу связи, таймаутов и прочего
    // nowTimeStamp.tick();
    hub.tick();
    UpTimeStamp.tick();
    data.tick();
    // eb.tick(); // энкодер если подключен
    // listen_eb();

    if (!digitalRead(BUT)) // активация, деактивация колес
    {
        delay(20);
        if (!digitalRead(BUT))
        {
            allowRunMoto = !allowRunMoto;
            digitalWrite(LED_BOARD, allowRunMoto);
            while (!digitalRead(BUT))
                delay(50);
        }
    }

    // получаем параметры ( скорость) по udp
    parseUdpMessage();
    setSpeed(speed); // вращаем колесами

    Serial.print("\trecievedSpeed ");
    Serial.print(recievedSpeed);
    Serial.print("\tspeed ");
    Serial.println(speed);

    switch (car_proc)
    {
    case 0:
        carMs = ms;
        car_proc = 3;
        break;
    case 3: // машина останавливается
        speed = 0;
        car_proc = 5;
        break;
    case 5: // машина  ждет получения скорости и страгивается
        if (recievedSpeed > 0)
        {
            speed = startSpeed;
            carMs = ms;
            car_proc = 7;
        }
        break;
    case 7: // ждем пока стронется
        if ((ms - carMs) > startTime)
        {
            if (!recievedSpeed) // скорость снова ноль? на исход
                car_proc = 3;
            else if (recievedSpeed < startSpeed) // скорость супер маленькая - сразу ее ставим
                speed = recievedSpeed;
            carMs = ms;
            car_proc = 10;
        }
    case 10: // стремимся достигнуть текущей скорости
        if ((ms - carMs) > incrSpeedStep)
        {
            carMs = ms;
            if (!slowBrake && !recievedSpeed) // если резкий тормоз активирован
            {
                car_proc = 3;
            }
            if (recievedSpeed < speed)
                speed--;
            else if (recievedSpeed > speed)
                speed++;
            if (speed < minSpeed) // скорость снова ноль? на исход
                car_proc = 3;
        }
        break;
    }

    // =========== ОБНОВЛяем интерфейс ПО ТАЙМЕРУ ===========
    static gh::Timer each1sec(1000); // период 1 секунда
    if (each1sec)
    {

        if (allowRunMoto)
        {
            hub.update("gas").color(0x71abea).value(speed); // обновин скорость в интерфейсе
        }
        else
        {
            hub.update("gas").color(0x6d4f4c).value(speed);
        }
        hub.update(F("inputed")).value(speed);
        // hub.update(F("inputed")).value( inp.toInt());
        hub.sendUpdate(F("uptime"));
        // hub.sendUpdate(F("datime;uptime"));
    } // each1sec

    // поддержка wifi соединения
    static gh::Timer each10sec(10000); // период 10 секунд
    if (each10sec)
    {
        wifiSupport();
    }

    /*/обработка входящих данных от CLI
      hub.onCLI([](String & str) {
      Serial.println(str);
      hub.printCLI("прочитано: " + str);
      });
    */

} // loop