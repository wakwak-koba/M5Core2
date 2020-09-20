#ifndef _TOUCH_H_
#define _TOUCH_H_

#include <Arduino.h>
#include <Wire.h>
#include <functional>
#undef min
#include <deque>

#define CST_DEVICE_ADDR 0x38

struct TouchPoint_t
{
    int x;
    int y;
    int event = -1;        // 0:PressDown 1:LiftUp 2:coodinate
    unsigned long ticks;
    
    inline std::string toString() {
        std::string res = "point:(";
        char val[10];
        snprintf(val, sizeof(val), "%03d,%03d", x, y);
        res += val;
        res += ")";
        res += ",event:";
        snprintf(val, sizeof(val), "%d", event);
        res += val;
        res += ",ticks:";
        snprintf(val, sizeof(val), "%lu", ticks);
        res += val;
        return res;
    }
};

struct HotZone_t
{
    virtual inline bool inHotZone(TouchPoint_t point) = 0;
    virtual inline bool inHotZoneDoFun(TouchPoint_t point)
    {
        if(_fun != nullptr && inHotZone(point)) {
            _fun();
            return true;
        }
        return false;
    }

    bool               enabled = true;
    std::string        name;
    void (*_fun)();

    struct EventArgs_t {
        TouchPoint_t point;
        std::deque<TouchPoint_t*> locusPoints;
        bool    suppress = true;
    };
    
    std::function<void(HotZone_t*, EventArgs_t*)> onClick;
    std::function<void(HotZone_t*, EventArgs_t*)> onPressChanged;
    std::function<void(HotZone_t*, EventArgs_t*)> onMove;
    
    virtual inline std::string toString() = 0;

};

struct HotZoneRect_t : public HotZone_t
{
    HotZoneRect_t(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, void (*fun)() = nullptr){
        _x0 = x0; 
        _y0 = y0;
        _x1 = x1;
        _y1 = y1;
        _fun = fun;

    }
    HotZoneRect_t(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, std::string name) : HotZoneRect_t(x0, y0, x1, y1) {
        this->name = name;
    }
    inline void setZone(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, void (*fun)() = nullptr )
    {
        _x0 = x0; 
        _y0 = y0;
        _x1 = x1;
        _y1 = y1;
        _fun = fun;
    }
    virtual inline bool inHotZone(TouchPoint_t point)
    {
        if(( point.x >= _x0 )&&( point.x <=  _x1 )&&
           ( point.y >= _y0 )&&( point.y <=  _y1 ))
        {
            return true;
        }
        return false;
    }
    uint16_t _x0;
    uint16_t _y0; 
    uint16_t _x1;
    uint16_t _y1;

    virtual inline std::string toString() {
        std::string res = "rect:(";
        char val[8];
        snprintf(val, sizeof(val), "%03d,%03d", _x0, _y0);
        res += val;
        res += ")-(";
        snprintf(val, sizeof(val), "%03d,%03d", _x1, _y1);
        res += val;
        res += ")";
        return res;
    }
};

struct HotZoneCircle_t : public HotZone_t
{
    HotZoneCircle_t(uint16_t x, uint16_t y, uint16_t r, void (*fun)() = nullptr){
        this->x = x;
        this->y = y;
        this->r = r;
        _fun = fun;

    }
    HotZoneCircle_t(uint16_t x, uint16_t y, uint16_t r, std::string name) : HotZoneCircle_t(x, y, r) {
        this->name = name;
    }

    virtual inline bool inHotZone(TouchPoint_t point)
    {
        if((point.x - x) * (point.x - x) + (point.y - y) * (point.y - y) <= r * r)
            return true;

        return false;
    }

    uint16_t x;
    uint16_t y;
    uint16_t r;

    virtual inline std::string toString() {
        std::string res = "circle:(";
        char val[12];
        snprintf(val, sizeof(val), "%03d,%03d,%03d", x, y, r);
        res += val;
        res += ")";
        return res;
    }
};

struct HotZoneTriangle_t : public HotZone_t
{
    HotZoneTriangle_t(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, void (*fun)() = nullptr){
        this->x0 = x0;
        this->y0 = y0;
        this->x1 = x1;
        this->y1 = y1;
        this->x2 = x2;
        this->y2 = y2;
        _fun = fun;

    }
    HotZoneTriangle_t(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, std::string name) : HotZoneTriangle_t(x0, y0, x1, y1, x2, y2) {
       this->name = name;
    }

    virtual inline bool inHotZone(TouchPoint_t point)
    {
        double a = 0.5 * (-y1 * x2 + y0 * (-x1 + x2) + x0 * (y1 - y2) + x1 * y2);
        double s = 1 / (2 * a) * (y0 * x2 - x0 * y2 + (y2 - y0) * point.x + (x0 - x2) * point.y);
        double t = 1 / (2 * a) * (x0 * y1 - y0 * x1 + (y0 - y1) * point.x + (x1 - x0) * point.y);
        
        if((0 < s < 1) && (0 < t < 1) && (0 < 1 - s - t < 1))
            return true;
        
        return false;
    }

    uint16_t x0;
    uint16_t y0;
    uint16_t x1;
    uint16_t y1;
    uint16_t x2;
    uint16_t y2;

    virtual inline std::string toString() {
        std::string res = "triangle:(";
        char val[8];
        snprintf(val, sizeof(val), "%03d,%03d", x0, y0);
        res += val;
        res += ")-(";
        snprintf(val, sizeof(val), "%03d,%03d", x1, y1);
        res += val;
        res += ")-(";
        snprintf(val, sizeof(val), "%03d,%03d", x2, y2);
        res += val;
        res += ")";
        return res;
    }
};

struct HotZoneBtn_t : public HotZoneRect_t
{
    HotZoneBtn_t(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, std::string name) : HotZoneRect_t(x0, y0, x1, y1, name) {
        ;
    }
public:
    uint8_t read();
    uint8_t isPressed();
    uint8_t isReleased();
    uint8_t wasPressed();
    uint8_t wasReleased();
    uint8_t pressedFor(uint32_t ms);
    uint8_t releasedFor(uint32_t ms);
    uint8_t wasReleasefor(uint32_t ms);
    uint32_t lastChange();
private:
};

class touch
{
private:
    /* data */

    volatile static unsigned long state;
public:
    
    TouchPoint_t _TouchPoint;
    
public:
    touch(/* args */);
    ~touch();
    void begin();
    bool ispressed();
    TouchPoint_t getPressPoint();
    HotZoneRect_t* createHotZone(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h);
private:
    int readTouchtoBuff(uint16_t *posx, uint16_t *posy);

public:
    std::function<void(HotZone_t*, HotZone_t::EventArgs_t*)> onClick;
    std::function<void(HotZone_t*, HotZone_t::EventArgs_t*)> onPressChanged;
    std::function<void(HotZone_t*, HotZone_t::EventArgs_t*)> onMove;
    
    HotZoneBtn_t BtnA {  0, 240,        119, UINT16_MAX, "buttonA"};
    HotZoneBtn_t BtnB {180, 240,        209, UINT16_MAX, "buttonB"};
    HotZoneBtn_t BtnC {240, 240, UINT16_MAX, UINT16_MAX, "buttonC"};

    HotZoneCircle_t* createHotZone(uint16_t x, uint16_t y, uint16_t r);
    HotZoneTriangle_t* createHotZone(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);

    HotZoneRect_t* addHotZone(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h, std::string name = {});
    HotZoneCircle_t* addHotZone(uint16_t x, uint16_t y, uint16_t r, std::string name = {});
    HotZoneTriangle_t* addHotZone(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, std::string name = {});

    bool addHotZone(HotZone_t* zone);
    bool removeHotZone(HotZone_t* zone, bool del = true);
    void handle();

private:
    HotZoneRect_t baseZone {0, 0, UINT16_MAX, 239, "somewhere"};
    std::deque<HotZone_t*> m_hotZones;
    std::deque<TouchPoint_t> m_locus[2];

    int readTouchtoBuff(TouchPoint_t *point1, TouchPoint_t *point2);
    static int calcDistance(TouchPoint_t* pos1, TouchPoint_t* pos2);
};


#endif