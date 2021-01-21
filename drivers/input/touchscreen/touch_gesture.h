extern uint8_t DouTap_enable;               // double tap
extern uint8_t UpVee_enable;               // ^
extern uint8_t DownVee_enable;             // v
extern uint8_t LeftVee_enable;              // <
extern uint8_t RightVee_enable;             // >
extern uint8_t Circle_enable;               // O
extern uint8_t DouSwip_enable;              // ||
extern uint8_t Up2DownSwip_enable;          // |v
extern uint8_t Down2UpSwip_enable;          // |^
extern uint8_t Mgestrue_enable;             // M

#define UnknownGesture       0
#define DouTap              0xcc   // double tap
#define UpVee               0x76   // V
#define DownVee             0x5e   // ^
#define LeftVee             0x3e   // >
#define RightVee            0x63   // <
#define Circle              0x6f   // O
#define DouSwip             0x48   // ||
#define Up2DownSwip         0xab  // |v
#define Down2UpSwip         0xba  // |^
#define Mgestrue            0x6d  // M

#define KEY_GESTURE_W               246
#define KEY_GESTURE_M               247
#define KEY_GESTURE_S               248
#define KEY_DOUBLE_TAP              KEY_WAKEUP
#define KEY_GESTURE_CIRCLE          250
#define KEY_GESTURE_TWO_SWIPE       251
#define KEY_GESTURE_UP_ARROW        252
#define KEY_GESTURE_LEFT_ARROW      253
#define KEY_GESTURE_RIGHT_ARROW     254
#define KEY_GESTURE_DOWN_ARROW      255
#define KEY_GESTURE_SWIPE_DOWN      KEY_F6
#define KEY_GESTURE_SWIPE_UP        KEY_F8
#define KEY_GESTURE_SINGLE_TAP      KEY_F9
