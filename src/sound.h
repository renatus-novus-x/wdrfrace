#ifndef WDR_SOUND_H
#define WDR_SOUND_H

class SoundEffect {
 private:
  enum Effect {
    EFFECT_NONE,
    EFFECT_CONFIRM,
    EFFECT_CANCEL,
    EFFECT_SELECT,
  };

  int initialized_;
  Effect effect_;
  int ticks_;

  void key_off();
  void play_note(int key_code);
  void start(Effect effect, int key_code);

 public:
  SoundEffect();
  void initialize();
  void update();
  void play_confirm();
  void play_cancel();
  void play_select();
  void finalize();
};

#endif
