
#include <M5Core2.h>

void onMove(HotZone_t* zone, HotZone_t::EventArgs_t* e)  {
  Serial.printf("onMove [%s] %d ", zone->name.c_str(), e->locusPoints.size());
  Serial.println(e->point.toString().c_str());  
  if(e->point.event == 1) // when LiftUp
    for (auto itp = e->locusPoints.begin(); itp != e->locusPoints.end(); ++itp)  {
      Serial.print("\t");
      Serial.println((*itp)->toString().c_str());
    }
}

void setup() {
  M5.begin(true, false, true, false);
  Serial.begin(115200);
  Serial.println();

  auto myArea1 = M5.Touch.addHotZone(0, 0, 99, 99, "top-left "); // rectangle
  auto myArea2 = M5.Touch.addHotZone(290, 50, 50, "top-right");  // circle
  
  M5.Touch.onMove = onMove;
// You can also disable the above and enable the following
/*M5.Touch.BtnA.onMove = onMove;
  M5.Touch.BtnB.onMove = onMove;
  M5.Touch.BtnC.onMove = onMove;
  myArea1->onMove = onMove;
  myArea2->onMove = onMove;*/
}

void loop() {
  M5.Touch.handle();
}
