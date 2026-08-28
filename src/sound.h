#ifndef WDR_SOUND_H
#define WDR_SOUND_H

class SoundEffect {
 private:
  enum Effect {
    EFFECT_CONFIRM,
    EFFECT_CANCEL,
    EFFECT_SELECT,
    EFFECT_COUNTDOWN,
    EFFECT_START,
    EFFECT_BOOST,
    EFFECT_DRIFT,
    EFFECT_TACKLE,
    EFFECT_WALL,
    EFFECT_GATE,
    EFFECT_SLIPSTREAM,
    EFFECT_COUNT,
  };

  int initialized_;
  void start(Effect effect);

 public:
  SoundEffect();
  void initialize();
  void update();
  void stop();
  void play_confirm();
  void play_cancel();
  void play_select();
  void play_countdown();
  void play_start();
  void play_boost();
  void play_drift();
  void play_tackle();
  void play_wall();
  void play_gate();
  void play_slipstream();
  int play(const char *label);
  static int label_count();
  static const char *label_at(int index);
  void finalize();
};

#endif
