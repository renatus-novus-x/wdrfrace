#ifndef WDR_SOUND_H
#define WDR_SOUND_H

class SoundEffect {
 private:
  enum Effect {
    EFFECT_NONE,
    EFFECT_CONFIRM,
    EFFECT_CANCEL,
    EFFECT_SELECT,
    EFFECT_COUNTDOWN,
    EFFECT_START,
    EFFECT_FINAL_LAP,
    EFFECT_GOAL_P1,
    EFFECT_GOAL_P2,
    EFFECT_GOAL_DRAW,
  };

  int initialized_;
  Effect effect_;
  const int *sequence_;
  int sequence_length_;
  int sequence_index_;
  int note_ticks_;

  void key_off();
  void play_note(int key_code);
  void start(Effect effect, const int *sequence, int length);

 public:
  SoundEffect();
  void initialize();
  void update();
  void play_confirm();
  void play_cancel();
  void play_select();
  void play_countdown();
  void play_start();
  void play_final_lap();
  void play_goal(int result);
  int play(const char *label);
  static int label_count();
  static const char *label_at(int index);
  void finalize();
};

#endif
