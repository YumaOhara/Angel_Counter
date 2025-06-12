#include <M5Stack.h>
#include <driver/pcnt.h>

//経過時間
unsigned long resetTime = 0;

//一時停止機能
bool isPaused = false;
unsigned long pauseStartTime = 0;
unsigned long pauseDuration = 0;

//カウンタ
const int pulsePins[5] ={36, 26, 16, 17, 35};
const pcnt_unit_t pcntUnits[5] = {
  PCNT_UNIT_0, PCNT_UNIT_1, PCNT_UNIT_2, PCNT_UNIT_3, PCNT_UNIT_4
};

int16_t pulseCounts[5] = {0, 0, 0, 0, 0};

TFT_eSprite spr = TFT_eSprite(&M5.Lcd);

// setupPCNT関数 (変更なし)
void setupPCNT(int index) {
  pcnt_config_t pcnt_config;
  pcnt_config.pulse_gpio_num = pulsePins[index];
  pcnt_config.ctrl_gpio_num = PCNT_PIN_NOT_USED;
  pcnt_config.channel = PCNT_CHANNEL_0;
  pcnt_config.unit = pcntUnits[index];
  pcnt_config.pos_mode = PCNT_COUNT_DIS;
  pcnt_config.neg_mode = PCNT_COUNT_INC;
  pcnt_config.lctrl_mode = PCNT_MODE_KEEP;
  pcnt_config.hctrl_mode = PCNT_MODE_KEEP;
  pcnt_config.counter_h_lim = 32767;
  pcnt_config.counter_l_lim = -32768;

  pcnt_unit_config(&pcnt_config);
  pcnt_counter_pause(pcntUnits[index]);
  pcnt_counter_clear(pcntUnits[index]);
  pcnt_counter_resume(pcntUnits[index]);
}

void setUI(){
  M5.Lcd.clearDisplay();
  M5.Lcd.display();
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.println("Angel Counter");
  M5.Lcd.setCursor(30,210);
  M5.Lcd.printf("Reset");
}

void setup() {
  M5.begin();
  M5.Lcd.setTextSize(2);
  setUI();
  M5.Lcd.setCursor(215,210);
  M5.Lcd.printf("STOP");

  spr.createSprite(320, 160);

  for (int i = 0; i < 5; i++) {
    setupPCNT(i);
  }

  resetTime = millis(); // 初期化時にリセット時間を設定
}

void loop() {
  M5.update();

  if(M5.BtnC.wasPressed()){
    isPaused = !isPaused; //toggle
    if (isPaused) {
      pauseStartTime = millis(); // 一時停止時間を記録
      for(int i = 0; i < 5; i++) {
        pcnt_counter_pause(pcntUnits[i]); // カウンターを一時停止
      }
      setUI();
      M5.Lcd.setCursor(215,210);
      M5.Lcd.printf("RESUME");
    } else {
      pauseDuration = millis() - pauseStartTime; // 一時停止時間の計算
      resetTime += pauseDuration; // 経過時間に一時停止時間を加算
      for(int i = 0; i < 5; i++) {
        pcnt_counter_resume(pcntUnits[i]); // カウンターを再開
      }
      setUI();
      M5.Lcd.setCursor(215,210);
      M5.Lcd.printf("STOP");
    }
  }

  // カウンター値を取得
  if(!isPaused) {
    for (int i = 0; i < 5; i++) {
      pcnt_get_counter_value(pcntUnits[i], &pulseCounts[i]);
    }
  }

  //経過時間を計算
  unsigned long elapsedTime;
  if (isPaused) {
    elapsedTime = pauseStartTime - resetTime;
  } else {
    elapsedTime = millis() - resetTime;
  }
  unsigned long hour = elapsedTime / 3600000;
  unsigned long min = (elapsedTime % 3600000) / 60000;
  unsigned long sec = (elapsedTime % 60000) / 1000;

  spr.fillSprite(TFT_BLACK);
  spr.setTextSize(2);
  for (int i = 0; i < 5; i++) {
    spr.setCursor(10, 28 + i * 28);
    spr.printf("CH %d (GPIO %d): %d", i, pulsePins[i], pulseCounts[i]);
  }
  spr.setCursor(120, 0); // 少しスペースを空ける
  spr.printf("Time:%02lu:%02lu:%02lu", hour, min, sec);
  spr.pushSprite(0, 30);
  
  // ボタンAが押されたらカウンターをリセット
  if(M5.BtnA.wasPressed()) {
    M5.Speaker.setVolume(3);
    M5.Speaker.tone(661, 200);
    for (int i = 0; i < 5; i++) {
      pcnt_counter_clear(pcntUnits[i]);
      pulseCounts[i] = 0; 
      if (isPaused) { // もし停止中にリセットされたら
        pcnt_counter_resume(pcntUnits[i]); // カウンタを再開状態に戻しておく
        setUI();
        M5.Lcd.setCursor(215,210);
        M5.Lcd.printf("STOP");
      }
    }
    resetTime = millis();
    isPaused = false;
    pauseDuration = 0;
    pauseStartTime = 0;
  }

  delay(50);
}