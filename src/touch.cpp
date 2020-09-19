#include "touch.h"

volatile unsigned long touch::state = 0;

touch::touch(/* args */)
{
    //Wire1.begin(21,22);  
    addHotZone(&baseZone);
    addHotZone(&BtnA);
    addHotZone(&BtnB);
    addHotZone(&BtnC);    
}
touch::~touch()
{

}

void touch::begin()
{
    Wire1.begin(21,22);  
    pinMode(39, INPUT);
    attachInterrupt(39, [] {
        unsigned long ticks = ((millis() >> 1) << 1) | (digitalRead(39) & 0x01);
        touch::state = ticks;
    }, CHANGE);

    Wire1.beginTransmission(CST_DEVICE_ADDR);
    Wire1.write(0xA4);
    Wire1.write(0x00);        // Interrupt Polling mode
    Wire1.endTransmission();
}

bool touch::ispressed()
{
    //bool touch;
    uint16_t pos_x,pos_y;
    //touch = ( digitalRead(39) == LOW ) ? true : false;
    if(  readTouchtoBuff(&pos_x,&pos_y) == 0x02 )
    {
        return true;
    }
    else
    {
        return false;
    }

    return false;
}

TouchPoint_t touch::getPressPoint()
{
    uint16_t pos_x,pos_y;
    if(  readTouchtoBuff(&pos_x,&pos_y) == 0x02 )
    {
        _TouchPoint.x = pos_x;
        _TouchPoint.y = pos_y;
        //Serial.printf("Touch %d:%d\r\n",_TouchPoint.x, _TouchPoint.y);
    }
    else
    {
        _TouchPoint.x = _TouchPoint.y = -1;
    }
    return _TouchPoint;
}

int touch::readTouchtoBuff(uint16_t *posx, uint16_t *posy)
{
    uint8_t buff[5];
    Wire1.beginTransmission(CST_DEVICE_ADDR);
    Wire1.write(0x02);
    if (Wire1.endTransmission() != 0)
        return -1;
    Wire1.requestFrom(CST_DEVICE_ADDR, 5);
    for (int i = 0; i < 5; i++)
    {
        buff[i] = Wire1.read();
        //Serial.printf("%02X ", buff[i]);
    }
    //Serial.println(".");
    if (buff[0] & 0x01)
    {
        *posx = ((buff[1] << 8) | buff[2]) & 0x0fff;
        *posy = ((buff[3] << 8) | buff[4]) & 0x0fff;
        return (buff[1] >> 6) & 0x03;
    }
    return 0;
}

HotZoneRect *touch::createHotZone(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h)
{
    return new HotZoneRect(x0, y0, x0 + w - 1, y0 + h - 1);
}

HotZoneCircle* touch::createHotZone(uint16_t x, uint16_t y, uint16_t r)
{
    return new HotZoneCircle(x, y, r);
}

HotZoneTriangle* touch::createHotZone(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    return new HotZoneTriangle(x0, y0, x1, y1, x2, y2);
}

HotZoneRect *touch::addHotZone(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h)
{
    HotZoneRect* zone = new HotZoneRect(x0, y0, x0 + w - 1, y0 + h - 1);
    if(addHotZone(zone))
        return zone;
    delete zone;
    return nullptr;
}

HotZoneCircle* touch::addHotZone(uint16_t x, uint16_t y, uint16_t r)
{
    HotZoneCircle* zone = new HotZoneCircle(x, y, r);
    if(addHotZone(zone))
        return zone;
    delete zone;
    return nullptr;
}

HotZoneTriangle* touch::addHotZone(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    HotZoneTriangle* zone = new HotZoneTriangle(x0, y0, x1, y1, x2, y2);
    if(addHotZone(zone))
        return zone;
    delete zone;
    return nullptr;
}

int touch::readTouchtoBuff(TouchPoint_t *point1, TouchPoint_t *point2)
{
    uint8_t buff[16];
    Wire1.beginTransmission(CST_DEVICE_ADDR);
    Wire1.write(0x00);
    if (Wire1.endTransmission() != 0)
        return -1;
    Wire1.requestFrom(CST_DEVICE_ADDR, sizeof(buff));
    for (int i = 0; i < sizeof(buff); i++)
        buff[i] = Wire1.read();
    
    unsigned long ticks = millis();
    if (point1)
    {
        point1->x = ((buff[3] << 8) | buff[4]) & 0x0fff;
        point1->y = ((buff[5] << 8) | buff[6]) & 0x0fff;
        point1->event = (buff[3] >> 6) & 0x03;
        point1->ticks = ticks;
    }
    if (point2)
    {
        point2->x = ((buff[9] << 8) | buff[10]) & 0x0fff;
        point2->y = ((buff[11] << 8) | buff[12]) & 0x0fff;
        point2->event = (buff[9] >> 6) & 0x03;
        point2->ticks = ticks;
    }
    return buff[2];
}

int touch::calcDistance(TouchPoint_t* pos1, TouchPoint_t* pos2)
{
    // root2 は取ってない
    return (pos1->x - pos2->x) * (pos1->x - pos2->x) + (pos1->y - pos2->y) * (pos1->y - pos2->y);
}

bool touch::addHotZone(HotZone* zone)
{
    m_hotZones.push_front(zone);
    return true;
}

bool touch::removeHotZone(HotZone* zone, bool del)
{
    for(auto it = m_hotZones.begin(); it != m_hotZones.end(); ++it) {
        if(*it == zone) {
            m_hotZones.erase(it);
            if(del)
                delete *it;
            return true;
        }
    }
    return false;
}

void touch::handle()
{
    static unsigned long last_state = 0;
    unsigned long now_state = state;
   
    // 暫定
    if((now_state & 0x01) == 0)
        return;
    
    if(last_state != now_state)
    {
        static int last_count;
        
        struct touchPoint {
            TouchPoint_t point;
            std::deque<TouchPoint_t>* locus = nullptr;
        };
        touchPoint nowTouch[2];
        bool clear[2] = {false, false};
        int now_count = readTouchtoBuff(&nowTouch[0].point, &nowTouch[1].point);
        
        // 軌跡を格納する
        if(now_count == 2)    {
            if(m_locus[0].size() == 0 && m_locus[1].size() == 0)    {
                // 0指→2指
                nowTouch[0].locus = &m_locus[0];
                nowTouch[1].locus = &m_locus[1];
                
            } else if(m_locus[0].size() > 0)    {
                // 1指→2指
                auto last_p1 = m_locus[0].end();
                int d1 = calcDistance(&(*last_p1), &nowTouch[0].point);
                int d2 = calcDistance(&(*last_p1), &nowTouch[1].point);
                if(d1 <= d2)    {
                    nowTouch[0].locus = &m_locus[0];
                    nowTouch[1].locus = &m_locus[1];
                } else {
                    nowTouch[0].locus = &m_locus[1];
                    nowTouch[1].locus = &m_locus[0];
                }
            } else if(m_locus[1].size() > 0)    {
                // 1指→2指
                auto last_p2 = m_locus[1].end();
                int d1 = calcDistance(&(*last_p2), &nowTouch[0].point);
                int d2 = calcDistance(&(*last_p2), &nowTouch[1].point);
                if(d1 <= d2)    {
                    nowTouch[0].locus = &m_locus[1];
                    nowTouch[1].locus = &m_locus[0];
                } else {
                    nowTouch[0].locus = &m_locus[0];
                    nowTouch[1].locus = &m_locus[1];
                }
            }
        } else if(now_count == 1)    {
            if(m_locus[0].size() == 0 && m_locus[1].size() == 0)    {
                // 0指→1指
                nowTouch[0].locus = &m_locus[0];
            } else if (m_locus[0].size() > 0 && m_locus[1].size() > 0)    {
                // 2指→1指(近い方を探す)
                auto last_p1 = m_locus[0].end();
                auto last_p2 = m_locus[1].end();
                auto d1 = calcDistance(&(*last_p1), &nowTouch[0].point);
                auto d2 = calcDistance(&(*last_p2), &nowTouch[0].point);
                if(d1 <= d2)    {
                    nowTouch[0].locus = &m_locus[0];
                    nowTouch[1].locus = &m_locus[1];
                    clear[1] = true;
                } else {
                    nowTouch[0].locus = &m_locus[1];
                    nowTouch[1].locus = &m_locus[0];
                    clear[0] = true;
                }
            } else if(m_locus[0].size() > 0)
                // 1指→1指
                nowTouch[0].locus = &m_locus[0];
            else if(m_locus[1].size() > 0)
                nowTouch[0].locus = &m_locus[1];
        } else if(now_count == 0)    {
            if(m_locus[0].size() > 0 && m_locus[1].size() > 0)    {
                // 2指→0指
                nowTouch[0].locus = &m_locus[0];
                nowTouch[1].locus = &m_locus[1];
            } else if(m_locus[0].size() > 0)    {
                // 1指→0指
                nowTouch[0].locus = &m_locus[0];
            }
            
            // N指→0指
            clear[0] = true;
            clear[1] = true;
        }
        
        for(int i = 0; i < 2; i++)    {
            if(nowTouch[i].locus != nullptr)    {
                if(nowTouch[i].locus->size() > 0)    {
                    auto last_p = nowTouch[i].locus->end();
                    if((*last_p).x != nowTouch[i].point.x || (*last_p).y != nowTouch[i].point.y)
                        nowTouch[i].locus->push_back(nowTouch[i].point);
                } else
                    nowTouch[i].locus->push_back(nowTouch[i].point);
            }
        }
        
        for(int i = 0; i < (last_count > now_count ? last_count : now_count); i++)    {
            for (auto it = m_hotZones.begin(); it != m_hotZones.end(); ++it)    {
                HotZone::eventArgs_T e;
                e.point = nowTouch[i].point;
                
                if(nowTouch[i].locus != nullptr)    {
                    for (auto itp = nowTouch[i].locus->begin(); itp != nowTouch[i].locus->end(); ++itp)    {
                        if((*it)->inHotZone((*itp)))
                            e.locusPoints.push_back(&(*itp));
                    }
                }
                
                // onMove
                    if((*it) != nullptr && (*it)->enabled && (*it)->inHotZone(e.point) && (*it)->onMove)
                    {
                        (*it)->onMove((*it), &e);
                        if(e.suppress)
                            break;
                    }
                    if((*it) != nullptr && (*it)->inHotZone(e.point) && this->onMove)
                    {
                        this->onMove((*it), &e);
                        if(e.suppress)
                            break;
                    }
                    if((*it) != nullptr && (*it)->enabled)
                        (*it)->inHotZoneDoFun(e.point);

                if(now_count != last_count)    {
                    // onClick
                    if((*it) != nullptr && (*it)->enabled && (*it)->inHotZone(e.point) && (*it)->onClick)
                    {
                        if(e.point.event == 1) {
                            auto f = e.locusPoints.front();
                            if(f->event == 0) {
                                (*it)->onClick((*it), &e);
                                if(e.suppress)
                                    break;
                            }
                        }
                    }
                    
                    // onPressChanged
                    if((*it) != nullptr && (*it)->enabled && (*it)->inHotZone(e.point) && (*it)->onPressChanged)
                    {
                        (*it)->onPressChanged((*it), &e);
                        if(e.suppress)
                            break;
                    }
                    if((*it) != nullptr && (*it)->inHotZone(e.point) && this->onPressChanged)
                    {
                        this->onPressChanged((*it), &e);
                        if(e.suppress)
                            break;
                    }
                }
            }
        }
        last_count = now_count;
        last_state = now_state;
        
        // clearLocus
        for(int i = 0; i < 2; i++)
            if(clear[i])
                m_locus[i].clear();
        
    }
}