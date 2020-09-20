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

HotZoneRect_t *touch::createHotZone(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h)
{
    return new HotZoneRect_t(x0, y0, x0 + w - 1, y0 + h - 1);
}

HotZoneCircle_t* touch::createHotZone(uint16_t x, uint16_t y, uint16_t r)
{
    return new HotZoneCircle_t(x, y, r);
}

HotZoneTriangle_t* touch::createHotZone(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    return new HotZoneTriangle_t(x0, y0, x1, y1, x2, y2);
}

HotZoneRect_t *touch::addHotZone(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h, std::string name)
{
    HotZoneRect_t* zone = new HotZoneRect_t(x0, y0, x0 + w - 1, y0 + h - 1, name);
    if(addHotZone(zone))
        return zone;
    delete zone;
    return nullptr;
}

HotZoneCircle_t* touch::addHotZone(uint16_t x, uint16_t y, uint16_t r, std::string name)
{
    HotZoneCircle_t* zone = new HotZoneCircle_t(x, y, r, name);
    if(addHotZone(zone))
        return zone;
    delete zone;
    return nullptr;
}

HotZoneTriangle_t* touch::addHotZone(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, std::string name)
{
    HotZoneTriangle_t* zone = new HotZoneTriangle_t(x0, y0, x1, y1, x2, y2, name);
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
    if (point1) {
        point1->x = ((buff[3] << 8) | buff[4]) & 0x0fff;
        point1->y = ((buff[5] << 8) | buff[6]) & 0x0fff;
        point1->event = (buff[3] >> 6) & 0x03;
        point1->ticks = ticks;
    }
    if (point2) {
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
    return ((int)pos1->x - (int)pos2->x) * ((int)pos1->x - (int)pos2->x) + ((int)pos1->y - (int)pos2->y) * ((int)pos1->y - (int)pos2->y);
}

bool touch::addHotZone(HotZone_t* zone)
{
    m_hotZones.push_front(zone);
    return true;
}

bool touch::removeHotZone(HotZone_t* zone, bool del)
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
    
    if(last_state != now_state) {
        static int last_count = 0;
        
        struct touchPoint {
            TouchPoint_t point;
            std::deque<TouchPoint_t>* locus = nullptr;
        };
        touchPoint nowTouch[2];
        int now_count = readTouchtoBuff(&nowTouch[0].point, &nowTouch[1].point);

/*      if(now_count > 0) {
            Serial.printf("%lu %d", nowTouch[0].point.ticks, now_count);
            Serial.println();
        }*/

        // 軌跡を格納する
        if(now_count == 2) {
            if(m_locus[0].size() == 0 && m_locus[1].size() == 0) {
                // 0指→2指
                nowTouch[0].locus = &m_locus[0];
                nowTouch[1].locus = &m_locus[1];
/*                Serial.printf("0_2");
                Serial.println();*/
                
            } else if(m_locus[0].size() > 0) {
                // 1指→2指
                auto last_p1 = m_locus[0].back();
                int d1 = calcDistance(&last_p1, &nowTouch[0].point);
                int d2 = calcDistance(&last_p1, &nowTouch[1].point);
/*                Serial.printf("1_2[0] (%03d,%03d) m_locus[0].size=%d d1:%d d2:%d", last_p1.x, last_p1.y, m_locus[0].size() , d1, d2);
                Serial.println();*/
                if(d1 <= d2) {
                    nowTouch[0].locus = &m_locus[0];
                    nowTouch[1].locus = &m_locus[1];
                } else {
                    nowTouch[0].locus = &m_locus[1];
                    nowTouch[1].locus = &m_locus[0];
                }
            } else if(m_locus[1].size() > 0) {
                // 1指→2指
                auto last_p2 = m_locus[1].back();
                int d1 = calcDistance(&last_p2, &nowTouch[0].point);
                int d2 = calcDistance(&last_p2, &nowTouch[1].point);
/*                Serial.printf("1_2[1] (%03d,%03d) m_locus[1].size=%d d1:%d d2:%d", last_p2.x, last_p2.y, m_locus[1].size() , d1, d2);
                Serial.println();*/
                if(d1 <= d2) {
                    nowTouch[0].locus = &m_locus[1];
                    nowTouch[1].locus = &m_locus[0];
                } else {
                    nowTouch[0].locus = &m_locus[0];
                    nowTouch[1].locus = &m_locus[1];
                }
            }
        } else if(now_count == 1) {
//            Serial.printf("\t%lu (%03d,%03d)\t", nowTouch[0].point.ticks, nowTouch[0].point.x, nowTouch[0].point.y);
            if(m_locus[0].size() == 0 && m_locus[1].size() == 0) {
                // 0指→1指
                nowTouch[0].locus = &m_locus[0];
//                Serial.printf("0_2");
            } else if (m_locus[0].size() > 0 && m_locus[1].size() > 0) {
                // 2指→1指(近い方を探す)
                auto last_p1 = m_locus[0].back();
                auto last_p2 = m_locus[1].back();
                auto d1 = calcDistance(&last_p1, &nowTouch[0].point);
                auto d2 = calcDistance(&last_p2, &nowTouch[0].point);
/*                Serial.printf("2_1[0] (%03d,%03d),(%03d,%03d) d1:%d d2:%d", last_p1.x, last_p1.y, last_p2.x, last_p2.y, d1, d2);
                Serial.println();*/
                if(d1 <= d2) {
                    nowTouch[0].locus = &m_locus[0];
//                    nowTouch[1].locus = &m_locus[1];
                } else {
                    nowTouch[0].locus = &m_locus[1];
//                   nowTouch[1].locus = &m_locus[0];
                }
            } else if(m_locus[0].size() > 0)
                // 1指→1指
                nowTouch[0].locus = &m_locus[0];
            else if(m_locus[1].size() > 0)
                nowTouch[0].locus = &m_locus[1];
        } else if(now_count == 0) {
            if(m_locus[0].size() > 0 && m_locus[1].size() > 0) {
                // 2指→0指
                auto last_p1 = m_locus[0].back();
                auto last_p2 = m_locus[1].back();
                auto d1 = calcDistance(&last_p1, &nowTouch[0].point);
                auto d2 = calcDistance(&last_p2, &nowTouch[0].point);
                if(d1 <= d2) {
                    nowTouch[0].locus = &m_locus[0];
                    nowTouch[1].locus = &m_locus[1];
                } else {
                    nowTouch[0].locus = &m_locus[1];
                    nowTouch[1].locus = &m_locus[0];
                }
            } else if(m_locus[0].size() > 0) {
                // 1指→0指
                nowTouch[0].locus = &m_locus[0];
            }
        }
        
        for(int i = 0; i < 2; i++) {
            if(nowTouch[i].locus != nullptr) {
                if(nowTouch[i].locus->size() > 0) {
                    auto last_p = nowTouch[i].locus->end();
                    if((*last_p).x != nowTouch[i].point.x || (*last_p).y != nowTouch[i].point.y || (*last_p).event != nowTouch[i].point.event)
                        nowTouch[i].locus->push_back(nowTouch[i].point);
                } else
                    nowTouch[i].locus->push_back(nowTouch[i].point);
            }
        }
        
        for(int i = 0; i < (last_count > now_count ? last_count : now_count); i++) {
            for (auto it = m_hotZones.begin(); it != m_hotZones.end(); ++it) {
                HotZone_t::EventArgs_t e;
                e.point = nowTouch[i].point;
                
                if(nowTouch[i].locus != nullptr) {
                    for (auto itp = nowTouch[i].locus->begin(); itp != nowTouch[i].locus->end(); ++itp) {
                        if((*it)->inHotZone((*itp)))
                            e.locusPoints.push_back(&(*itp));
                    }
                }
                
                // onMove
                    if((*it) != nullptr && (*it)->enabled && (*it)->inHotZone(e.point)) {
                        if((*it)->onMove) {
                            (*it)->onMove((*it), &e);
                            if(e.suppress)
                                break;
                        }
                        if(this->onMove) {
                            this->onMove((*it), &e);
                            if(e.suppress)
                                break;
                        }
                    }
                    if((*it) != nullptr && (*it)->enabled)
                        (*it)->inHotZoneDoFun(e.point);

                if(now_count != last_count) {
                    // onClick
                    if(e.point.event == 1) {
                        auto f = e.locusPoints.front();
                        if(f->event == 0) {
                            if((*it) != nullptr && (*it)->enabled && (*it)->inHotZone(e.point)) {
                                if((*it)->onClick) {
                                    (*it)->onClick((*it), &e);
                                    if(e.suppress)
                                        break;
                                }
                                if(this->onClick) {
                                    this->onClick((*it), &e);
                                    if(e.suppress)
                                        break;
                                }
                            }
                        }
                    }
                    
                    // onPressChanged
                    if((*it) != nullptr && (*it)->enabled && (*it)->inHotZone(e.point)) {
                        if((*it)->onPressChanged) {
                            (*it)->onPressChanged((*it), &e);
                            if(e.suppress)
                                break;
                        }
                        if(this->onPressChanged) {
                            this->onPressChanged((*it), &e);
                            if(e.suppress)
                                break;
                        }
                    }
                }
            }
        }
        for(int i = 0; i < 2; i++)
            if(m_locus[i].size() > 0) {
                auto last_p = m_locus[i].back();
                if(last_p.event == 1)    // LiftUp
                    m_locus[i].clear();
            }
        
        last_count = now_count;
        last_state = now_state;
    }
}