#ifndef _XONIX_H_
#define _XONIX_H_

#define IDD_NEW			0x0101
#define IDD_PAUSE		0x0102
#define IDD_EXIT		0x0103
#define IDD_ABOUT		0x0104

#define APP_CLASS		"XONIX"
#define APP_TITLE		"Xonix"

#define ACC_NEW			"^N"
#define ACC_PAUSE		VK_ESCAPE
#define ACC_EXIT		"^X"
#define ACC_ABOUT		"^A"

#define MNU_GAME		"Game"
#define MNU_NEW			"New\t Ctrl+N"
#define MNU_PAUSE		"Pause\t Esc"
#define MNU_EXIT		"Exit\t Ctrl+X"
#define MNU_HELP		"Help"
#define MNU_ABOUT		"About...\t Ctrl+A"

#define TXT_ABOUT		"Xonix for Windows 1.0"
#define TXT_COPYRIGHT	"© 2024 Stanislav Safronov"
#define TXT_PAUSED		"PAUSED"
#define TXT_NEXTLEVEL	"LEVEL COMPLETED"
#define TXT_GAMEOVER	"GAME OVER"
#define TXT_HIGHSCORE	"NEW HIGH SCORE"
#define TXT_LEVEL		"Level: %d"
#define TXT_LIVES		"Lives: %d"
#define TXT_SCORE		"Score: %ld"
#define TXT_HSCORE		"High score: %ld"

#define TMR_ID			1
#define TMR_DELAY		45

#endif
