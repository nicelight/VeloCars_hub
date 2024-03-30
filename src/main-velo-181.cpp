
// C:\Users\Acer\Documents\PlatformIO\Projects\veloCars_GyverHub\.pio\build\car
#include <Arduino.h>

#define MOT1_PIN 27
#define MOT2_PIN 26
#define SENSOR 16

#define RELE1 2
#define ON 1
#define OFF 0

#define MINSPEED 30       // минимальная скорость на которой машинка не остановится
#define MAXSPEED 250      // не больше 255
#define MINSLOWPERIOD 500 // время в милисекундах между пролетами магнита мимо датчика при самом медленном вращении педалями
#define MAXFASTPERIOD 100 // время в милисекундах между пролетами магнита мимо датчика при самом БЫСТРОМ вращении педалями

#define DREBEZG 2 // сколько милисекунд игнорировать переходные процессы. если педали работают глючно, увеличить значение до 4-10

// #define PEDALDEBUG // отладка датчика педалей

// const char *ssid = "esp32";
// const char *password = "123456789000";
// const char *ssid = "kuso4ek_raya";
// const char *password = "1234567812345678";

const char *ssid = "Showmix";
const char *password = "tol9igor";
#define STATIC_IP

#ifdef STATIC_IP
IPAddress staticIP(192, 168, 3, 181); // важно правильно указать третье число - подсеть, смотри пояснения выше
IPAddress gateway(192, 168, 3, 1);    // и тут изменить тройку на свою подсеть
IPAddress subnet(255, 255, 255, 0);
IPAddress dns1(192, 168, 10, 1); // изменить тройку на свою подсеть
IPAddress dns2(8, 8, 8, 8);
#endif

#include "AsyncUDP.h"
AsyncUDP udp_out;
const uint16_t port_out = 12171; // Определяем порт
IPAddress car171ip(192, 168, 3, 171);

#include <Stamp.h>
#include <StampTicker.h>
// #include <GyverNTP.h>
Datime nowDatime;
// Stamp fixedTimeStamp;
StampTicker nowTimeStamp;
StampTicker UpTimeStamp;

#define GH_INCLUDE_PORTAL // +4% к памяти программы и локальный портал присутствует.
#include <GyverHub.h>
GyverHub hub;

#include <PairsFile.h>
PairsFile data(&GH_FS, "/data.dat", 3000);

String minper;
String maxper;
bool flag = 0; //

#define LED 2
#define BUT 0
#define PWMCHAN1 0
#define PWMCHAN2 1

uint8_t pedals = 0; // автомат обработки датчика педалей
unsigned long
    ms = 0,
    pedalBounce = DREBEZG, // подавление переходных процессов
    pedalsMs = 0,
    prevTotalPeriod = 0,
    pedalNearSensorTime = 0, // как долго магнит находится рядом с датчиком в момент пролета (~~ 8-60 ms)
    prevPedalNearSensorTime = 0,
    periodMs = 0,
    speed = 0; // передаваемая машинке текущая скорость  ( рассчитанная + джойтсик)

byte gear = 1;        // передача 2 или 1
long joyAdd171 = 0;   // добавка к периоду педалей с джойстика
long totalPeriod = 0; // промежуточный рассчет периода с поправкой на джойстик
long addSpeed171 = 0; // добавка к конечной скорости

volatile unsigned long
    pedalPeriod = 0,            // период кручения педалей
    slowPeriod = MINSLOWPERIOD, // минимальный период вращения педалями
    fastPeriod = MAXFASTPERIOD; // максимальный период вращения педалями
volatile unsigned long
    countedSpeed = 0,    // рассчитанная скорость вращения машинки
    minSpeed = MINSPEED, // минимальная скорость машинки ( пока она еще может ехать, не останавливается)
    maxSpeed = MAXSPEED; // максимальная скорость машинки

volatile float A = 0.0; // коэффициент для рассчета скорости

// волейтилы не записываются в портал, используем перевалочные переменные
int slw = slowPeriod;
int fst = fastPeriod;

bool pedalsRUN = 0; // флаг, что педали крутят
bool needPrint = 0; // для печати отладочной скорости равной нулю

void calcA()
{
    // формула скорости в общем y = Ax + B
    //  в нашем случае  countedSpeed = (pedalPeriod + (joyAdd171) - fastPeriod)*A + minSpeed;
    // A = (maxSpeed - minSpeed)/(slowPeriod - fastPeriod)
    A = float(maxSpeed - minSpeed) / (slowPeriod - fastPeriod);
    // после чего считаем скорость
    // countedSpeed = maxSpeed - (pedalPeriod - fastPeriod + joyAdd171) * A;

    // speed = countedSpeed + addSpeed171; // добавка с джойстика
    Serial.println("\n\n\tcalc A");
    Serial.print("\tslowPeriod ");
    Serial.print(slowPeriod);
    Serial.print("\tfastPeriod ");
    Serial.print(fastPeriod);
    Serial.print("\t\tA= ");
    Serial.print(A);
    Serial.print("\n\n\n");
} // calcA

// Определяем callback функцию обработки udp пакета
void parseUdpPacket(AsyncUDPPacket packet)
{
    // Выводим в последовательный порт все полученные данные
    // хороший сайт для работы с библиотекой AsyncUdp
    // https://community.appinventor.mit.edu/t/esp32-with-udp-send-receive-text-chat-mobile-mobile-udp-testing-extension-udp-by-ullis-ulrich-bien/72664/2

    Serial.write(packet.data(), packet.length());

    Serial.println();
} // parseUdpPacket()

// подключение по udp
bool udpFine()
{
    Serial.print("try udp");
    // Если удалось подключится по UDP
    for (int i = 0; i < 5; i++)
    {
        if (!udp_out.connect(car171ip, port_out))
        {
            //   Serial.print(".");
            digitalWrite(LED, 1);
            delay(20);
            digitalWrite(LED, 0);
            delay(10);
            if (i == 4)
                return 0; // вернем ошибку, если не смогли подключиться за 4 попытки
        }
        else
            break;                    // выйдем из цикла, если подключились
    }                                 // for
                                      //   Serial.println("\nUDP fine");
    udp_out.onPacket(parseUdpPacket); // вызываем callback функцию при получении пакета
    return 1;
} // udpFine()

void send_udp_message()
{
    /*
      //альтернативный вариант отправки 1: отправляем udpMessage
      char udpMessage[20];
      sprintf(udpMessage, "%lu", second);
      Serial.println(udpMessage);
      udp.broadcastTo(udpMessage, port); //альтернативный вариант отправки 1

        // альтернативный вариант отправки 2: отправляем udpData
        //  number++;
        //  udpData += String((int)number);
        // udp.broadcastTo(udpData.c_str(), port); // альтерн вариант отправки 2
        //  udp.broadcastTo("1", port); // отправляем единичку (из примера с аквапарком)
    */

    if ((WiFi.status() == WL_CONNECTED) && (udp_out.connect(car171ip, port_out)))
    {

        String udpData = String((int)speed);
        udp_out.broadcastTo(udpData.c_str(), port_out); // альтерн вариант отправки 2
    }
} // send_udp_message()

// билдер
bool dsbl = 0, nolbl = 0, notab = 0; //  для билдера
void build(gh::Builder &b)
{

    {
        gh::Row r3(b);
        b.Time_(F("uptime"), &UpTimeStamp.unix).label(F("Аптайм"));
    }

    //    {
    //      gh::Row r5(b);
    //      b.Label_(F("dht22T")).size(2).label(F("За окном температура")).color(gh::Colors::Aqua);

    {
        gh::Row r3(b);
        b.Title_(F("title")).label(F("обн. из loop"));
    }
    // =================== ОБРАБОТКА КНОПОК ===================

    {
        gh::Row r(b);
        // Скорость педалей
        b.Gauge_("pedals").size(2).range(slw, fst, 1).unit(" педали").color(0x00fbff);
        // Скорость машинки
        b.Gauge_("gas").size(2).range(1, 255, 1).unit(" км/ч ").label(F("Машина 171")).color(0x71abea);
    }

    {
        gh::Row r7(b);
        // для сохранения параметров TODO
        if (b.Input(&minper).label(F("set медл. период")).size(1).click())
        {
            slw = minper.toInt();
            slowPeriod = slw;
            calcA();
        }
        if (b.Input(&maxper).label(F("set быстр период")).size(1).click())
        {
            fst = maxper.toInt();
            fastPeriod = fst;
            calcA();
        }
        b.Label_(F("slowperiod")).label(F("Медленный:")).size(1).value(slw); // с указанием стандартного значения
        b.Label_(F("fastperiod")).label(F("Быстрый:")).size(1).value(fst);   // с указанием стандартного значения
    }

    {
        static bool sw;
        gh::Row r8(b);
        // внутри обработки действия переменная уже будет иметь новое значение:
        if (b.Slider(&addSpeed171).label(F("прибавка к скорости 171й машины")).size(3).range(-20, 20, 1).click())
        {
            // Serial.print("sld: ");
            // Serial.println(sld);
        }

        if (b.Switch(&sw).label(F("нитро")).size(1).click())
        // if (b.Switch(&flag).label(F("активировать мотор")).size(1).click())
        {
            if (sw)
                gear = 2;
            else
                gear = 1;
        }
    }

    // ==================== ОБНОВЛЕНИЕ ====================

    // для отправки обновления нужно знать ИМЯ компонента. Его можно задать почти у всех виджетов
    // к функции добавляется подчёркивание, всё остальное - как раньше

    {
        gh::Row r9(b);
        // НАЖАЛИ КНОПКУ, ОБНОВИЛОСЬ ЗНАЧЕНИЕ РЯДОМ В ОКНЕ, в коде рядом
        // b.Label_(F("label")).size(2).value("default"); // с указанием стандартного значения

        // if (b.Button().size(1).click())
        // {
        //     hub.update(F("label")).value(random(100, 500));
        // }
    }

    {
        gh::Row r10(b);
        gh::Pos pos171;
        b.Joystick(&pos171).label(F("171 ускорим")).noTab(0).square(1);
        if (pos171.changed())
        {
            joyAdd171 = pos171.y;
            if (joyAdd171 > 500)
                joyAdd171 = 500;
#ifdef PEDALDEBUG
            Serial.print("joy 171:");
            Serial.println(pos171.y);
#endif
        }
    }
} // build()

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

void setup()
{
    Serial.begin(115200);

    wifiSupport();

    Stamp initStamp(0, 1, 1, 0, 0, 0);
    UpTimeStamp.update(initStamp.unix); // присваиваем аптайму штамп 1 января 0
    setStampZone(5);

    hub.config(F("MyDevices"), F("VeloCars181"), F("")); // 1 - префикс, для приложения, 2 - имя устройства
    hub.onBuild(build);

    // запуск!
    hub.begin();
    data.begin();
    // проверка записи в память
    data.set("key2", 3.14); 
    data.set("key3", F("abcd"));

    pinMode(BUT, INPUT);
    pinMode(LED, OUTPUT);

    pinMode(SENSOR, INPUT);
    calcA(); // рассчет коэффициента
} // 

void loop()
{
    hub.tick(); // вызываем тикер в главном цикле программы
    UpTimeStamp.tick();
    data.tick();
    // eb.tick(); // энкодер если подключен
    // listen_eb();

    if (!digitalRead(BUT))
    {
        delay(20);
        if (!digitalRead(BUT))
        {
            flag = !flag;
            digitalWrite(LED, flag);
            while (!digitalRead(BUT))
                delay(50);
        }
    }

    switch (pedals) // обработка датчика педалей
    {
    case 0: // инициализация
        pedals = 5;
        break;
    case 5: // момент первого подлета педали к датчику
        if (digitalRead(SENSOR))
        {
            pedalsMs = millis();
            pedals = 7;
        }
        else
            pedals = 0;

        break;
    case 7: // ожидаем пока пройдет дребезг и педаль находится у датчика более хх милисекунд
        if (digitalRead(SENSOR))
        {
            if ((millis() - pedalsMs) > pedalBounce)
            {

                pedalPeriod = millis() - prevPedalNearSensorTime; // посчитали новое время периода пролета магнита от педали до педали
                prevPedalNearSensorTime = millis();               // зафиксировали момент педали у датчика
                periodMs = millis();                              // обновляем при каждом подлете магнита. Если не обновляется дольше максимального времени, то сбрасываем подсчет скорости
                // надо тестировать, пока сомнительно работает
                // if (!pedalsRUN) // чтобы шустрее схватывало, если педаль не двигалась, то сделает что период минимальный уже 
                //     pedalPeriod = slowPeriod;

                pedalsMs = millis();
                pedals = 9;
            }
        }
        else            // еще дребезг или наводка
            pedals = 0; // на исходную
        break;
    case 9: // момент первого отлета педали от датчика
        if (!digitalRead(SENSOR))
        {
            pedalsMs = millis();
            pedals = 11;
        }
        break;
    case 11: // ожидаем пока пройдет дребезг и педаль отдалилась от датчика более хх милисекунд
        if (!digitalRead(SENSOR))
        {
            if ((millis() - pedalsMs) > pedalBounce)
            {
                pedalNearSensorTime = millis() - prevPedalNearSensorTime; // сколько времени магнит пролетал мимо датчика
                flag = 1;
                pedalsMs = millis();
                pedals = 5;
            }
        }
        else // еще дребезг
            pedals = 9;
        break;
    } // switch(pedals)

    ///////////////////////////////////////////////////////////////////
    ////////////////// 0. НАЧАЛО РАССЧЕТА СКОРОСТИ ////////////////////////
    ///////////////////////////////////////////////////////////////////

    //************** 0.1 если долго не приходили обновления от автомата обработки датчика педалей, -> уже не крутят
    if ((millis() - periodMs) > slowPeriod)
    {
        pedalPeriod = slowPeriod + 1; // выгоняем период за макисмальные рамки
        pedalsRUN = 0;                // флаг, что педали крутят

        // #ifdef PEDALDEBUG
        //             Serial.print("\n\n pedals STOP-ed \n\n ");
        // #endif
    }
    else
        pedalsRUN = 1;

    //**************************************************
    //**************** 1. РАССЧЕТ ПЕРИОДА *************
    //**************************************************

    //*** 1.2 проверим, что джойстик не накручен жоще, чем надо, нормализуем период c учетом джойстика
    int fromJoyPeriod;
    if (gear == 2) // если нитро активировано
        fromJoyPeriod = (0 - joyAdd171) * 2;
    else if (gear == 1)
        fromJoyPeriod = 0 - joyAdd171;

    if (fromJoyPeriod >= 0)
    {
        totalPeriod = pedalPeriod + fromJoyPeriod;
        if (totalPeriod > slowPeriod)
            totalPeriod = slowPeriod;
    }
    else if (fromJoyPeriod < 0)
    {
        if ((0 - fromJoyPeriod) < (pedalPeriod - fastPeriod))
            totalPeriod = pedalPeriod + fromJoyPeriod;
        else
            totalPeriod = fastPeriod;
    }

    //**************************************************
    ////////////////// 2. РАССЧЕТ СКОРОСТИ //////////////////
    //**************************************************
    //******* 2.1 по приходу нового значения периода, и если период не вылез за рамки, обсчитываем скорость
    if ((prevTotalPeriod != totalPeriod) && (totalPeriod <= slowPeriod))
    {
        prevTotalPeriod = totalPeriod;
        //*** 2.2 поcчитаем нескорректированную скорость
        countedSpeed = maxSpeed - (totalPeriod - fastPeriod) * A;
        //****2.3 нормализуем скорость корректировкой
        if (countedSpeed < minSpeed)
            countedSpeed = 0;
        if ((countedSpeed + addSpeed171) < 255)
        {
            speed = countedSpeed + addSpeed171; // КАЛИБРОВОЧНАЯ корректировка скорости
        }
        else // вышли за границы, не будем прибавлять надбавку скорости
        {
            speed = countedSpeed;
        }
#ifdef PEDALDEBUG

        Serial.print("\tcountedSpeed ");
        Serial.print(countedSpeed);
        Serial.print("\tTOTALspeed ");
        Serial.print(speed);
        Serial.print("\tjoyAdd171 ");
        Serial.print(joyAdd171);
        Serial.print("\tfromJoyPeriod ");
        Serial.print(fromJoyPeriod);
        Serial.print("\ttotalPeriod ");
        Serial.print(totalPeriod);
        Serial.print("\t\t\tpedalPeriod ");
        Serial.print(pedalPeriod);
        Serial.print("\t pedalNearSensorTime: ");
        Serial.print(pedalNearSensorTime);
        Serial.println();
        needPrint = 1;

#endif
    }                                  // if new period
    else if (totalPeriod > slowPeriod) // педали не крутят, джойстиком не мутят, скорость ноль
    {
        speed = 0;

#ifdef PEDALDEBUG
        if (needPrint)
        {
            Serial.print("\n\n pedals STOP-ed \n\n ");
            needPrint = 0;
        }
#endif
    }
    // *****************************************************************
    // ****************** кноец рассчета скорости **********************

    // отправляем данные  по udp
    send_udp_message();

    // =========== ОБНОВЛяем интерфейс каждую секунду
    static gh::Timer each1sec(1000); // период 1 секунда
    if (each1sec)
    {
        hub.update("gas").color(0x71abea).value(speed); // обновим скорость в интерфейсе
        int ped = pedalPeriod;
        hub.update("pedals").color(0x71abea).value(ped); // обновим частоту вращения педалей в интерфейсе
        // hub.update("gas").color(0x6d4f4c).value(speed);
        hub.update(F("title")).value(millis()); // каждую секунду будем обновлять заголовок
                                                // hub.update(F("slowperiod")).value(speed);

        // для сохранения параметров TODO
        hub.update(F("slowperiod")).value(slw);
        hub.update(F("fastperiod")).value(fst);
        hub.sendUpdate(F("uptime"));
        // hub.sendUpdate(F("datime;uptime"));

    } // each1sec

    // поддержка wifi соединения
    static gh::Timer each10sec(10000); // период 10 секунд
    if (each10sec)
    {
        wifiSupport();
    }

} // loop