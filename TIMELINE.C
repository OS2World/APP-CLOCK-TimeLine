/*--------------------------------------------------------------------------
  TIMELINE.C -- Time and Date Display for OS/2 Presentation Manager
                (c) 1989, Ziff Communications Co.
                PC Magazine * Charles Petzold, November 1988

                Modified March-April 1989 by Eric L. Baatz to add overly
                verbose comments, use of mouse for display movement, color
                changes and termination, a help file, use of OS2.INI and
                parameter recognition.  Those modifications in no way
                change the ownership of TIMELINE, the holding of any
                copyrights, or extend or offer any warranties as to the
                fitness or usefulness of this code, which is presented on
                an "as is basis" with no warranties.
  --------------------------------------------------------------------------*/

#define INCL_WIN
#define INCL_GPI
#include <os2.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include "timeline.h"

#define ID_TIMER   1   // Each timer a program starts must have a unique ID
#define MAX_COLORS 16  // The number of entries in the default color
                       // table through which TIMELINE will cycle the
                       // text background color when Button 2 is clicked
// Ignore status bits other than the following when a keystroke is received.
#define VKEY_MASK  (KC_KEYUP | KC_ALT | KC_CTRL | KC_SHIFT | KC_VIRTUALKEY)

// Function prototypes for handling communications with the date/time display
// window and the help dialog box.
MRESULT EXPENTRY ClientWndProc(HWND,     // msg result, exported entry,
                                         // handle for window
                               USHORT,   // unsigned short
                               MPARAM,   // message parameter
                               MPARAM);
MRESULT EXPENTRY HelpDlgProc( HWND, USHORT, MPARAM, MPARAM );


       HAB     hab;                       // handle to an anchor block (an
                                          // area of internal PM resources
                                          // that is allocated per thread
                                          // calling WinInitialize)
       HPS     hps;                       // handle to presentation space
       int     cArg;                      // count of TIMELINE parameters
       char  **ppchArg;                   // pointer to strings that are the parameters
static CHAR   szAppName[]  = "TimeLine2"; // string terminated by a zero
                                          // name of this application and
                                          // its window class
static CHAR   szKeyName1[] = "POSITION";  // keyword for writing into OS2.INI
static CHAR   szKeyName2[] = "COLOR";     // keyword for writing into OS2.INI



/*--------------------------------------------------------------------------

 When run from an OS/2 1.1 protected mode prompt or from a file such as
 STARTUP.CMD, TIMELINE displays a small rectangle on the Presentation
 Manager Desktop that shows the current date and time. The display is
 updated every second.

 TIMELINE can be terminated by double clicking Mouse Button 1 while the
 cursor is over the display or by pressing F3 or Alt+F4 when the display
 has input focus.

 The display can be moved around the PM Desktop by dragging it with Mouse
 Button 1 or by getting input focus to the display, pressing Alt+F7, pressing
 the arrow keys, and pressing Enter (to cause the display to move) or
 Esc (to cancel the move).
 
 The background color of the display can be cycled through the 16 default
 colors by clicking Mouse Button 2 while the cursor is over the display or
 by pressing the up- and down-arrow keys when the display has input focus.

 A simple help message is displayed if F1 is pressed while the display has
 input focus.

 Whenever the display's background color or position is changed, the new
 value(s) is written to OS2.INI.  If TIMELINE is started with no parameters,
 that is:

    TIMELINE

 then the display's initial color and position are read from OS2.INI.
 
 TIMELINE can be started with command line parameters in the form:

    TIMELINE x y color

 where

    x      is the initial x coordinate for the bottom, left corner of the
           PM Desktop.  If the value of x is negative or larger than the
           PM's display device, x will be adjusted so TIMELINE's display
           is visible on the Desktop.  (The point 0,0 is the bottom, left
           corner of the Desktop.)

    y      is the initial y coordinate for the bottom, left corner of the
           PM Desktop.

    color  is one of the following (case can be mixed):

           background, blue, red, pink, green, cyan, yellow, neutral,
           darkgray, darkblue, darkred, darkpink, darkgreen, darkcyan,
           brown, palegray

 For example:

    TIMELINE 451 0 DARKCYAN

 which puts the display in the bottom, right corner of a VGA display.


 The purpose of this program was to provide a platform for my first PM
 learning experience.  It turned out to be both fun and educational.
 This program is offered in the hope that it will help someone else get
 started.

 Thanks to Charles Petzold for TIMELINE's initial conception and for his
 many subsequent comments and pointers.

 However, any bugs, oversights, and stylistic offenses are entirely mine.
 I welcome feedback on them over CompuServe.

 Eric L. Baatz 74010,3664
 
 --------------------------------------------------------------------------*/



int main( int argc, char *argv[] )
{
  static ULONG fulFrameFlags =                   // flags ULONG
                               FCF_BORDER   |    // thin border
                               FCF_TASKLIST;     // add to PM task lst
  HMQ          hmq;                              // handle to message queue
  HWND         hwndFrame, hwndClient;            // handle to window
  QMSG         qmsg;                             // queued message

  cArg = argc;       // make visible to other functions
  ppchArg=argv;      // make visible to other functions

  
  hab = WinInitialize( (USHORT) 0 );             // parameter must be 0
  hmq = WinCreateMsgQueue( hab, (SHORT) 0 );     // use default queue size
  
  WinRegisterClass(hab,
                   szAppName,                    // class name
                   ClientWndProc,                // window procedure
                   (ULONG) 0,                    // class style
                   (USHORT) 0);                  // extra storage per window

  // Create the frame and client windows
  hwndFrame = WinCreateStdWindow(HWND_DESKTOP,   // parent window
                                 WS_VISIBLE,     // frame style
                                 &fulFrameFlags, // frame style flags
                                 szAppName,      // client class name
                                 NULL,           // title bar text
                                 (ULONG) 0,      // client window style
                                 NULL,           // resource identifier
                                 (USHORT) 0,     // frame window identifier
                                 &hwndClient);   // returned client window
                                                 // handle
  
  // Wait for a message.  If message is not WM_QUIT, dispatch it to
  // appropriate window procedure.
  while ( WinGetMsg(hab,
                    &qmsg,                       // returned pointer to msg
                    NULL,                        // window filter
                    (USHORT) 0,                  // first message identity
                    (USHORT) 0)                  // second message identity
        )
    WinDispatchMsg( hab, &qmsg );

  WinDestroyWindow( hwndFrame );
  WinDestroyMsgQueue( hmq );
  WinTerminate( hab );
  return( 0 );
}



VOID SizeWindow( HWND hwndFrame, PPOINTL pptlPosition )
{
  static CHAR szDummyTime[] = " Wed May 00 00:00:00 0000 ";  // "W" and "M"
                                                             // used so sizing
                                                             // will be for
                                                             // maximum length
                                                             // of a portion-
                                                             // ally spaced
                                                             // font
  POINTL      aptl[TXTBOX_COUNT];   // array of points (x,y coordinates)
  RECTL       rcl;                  // rectangle structure
  LONG        lDesktopX, lDesktopY;
  
  // Find width and height of text string
  
  GpiQueryTextBox(hps,
                  (LONG) strlen(szDummyTime), // number of chars in string
                  szDummyTime,                // the string
                  TXTBOX_COUNT,               // number of points to return
                                              // starting from TOPLEFT
                  aptl);                      // pointer to POINTLs that
                                              // will contain the relative
                                              // coordinates of the text box
                                              // in world coordinates
  
  // Set up rectangle structure that will hold the text and be completely
  // on the desktop

  //  xRight  = width of text box
  rcl.xRight  = aptl[TXTBOX_BOTTOMRIGHT].x - aptl[TXTBOX_BOTTOMLEFT].x;

  //  yTop    = height of text box
  rcl.yTop    = aptl[TXTBOX_TOPLEFT].y     - aptl[TXTBOX_BOTTOMLEFT].y;
  
  rcl.xLeft = rcl.yBottom = 0;
  WinCalcFrameRect(hwndFrame,                // calculate size and position
                   &rcl,                     // of frame necessary to display
                   FALSE);                   // text starting at 0,0
  rcl.xRight = rcl.xRight - rcl.xLeft;       // form relative width of frame
  rcl.yTop   = rcl.yTop   - rcl.yBottom;     // form relative height of frame

  // Get x dimension of the screen (desktop).
  if ( (lDesktopX = WinQuerySysValue( HWND_DESKTOP, SV_CXSCREEN )) == 0)
    lDesktopX = rcl.xRight;
  //  xLeft   = suggested x coordinate, unless that would cause clipping
  rcl.xLeft   = ( (pptlPosition->x + rcl.xRight) > lDesktopX )
                 ? lDesktopX - rcl.xRight : pptlPosition->x;
                 
  // Get y dimension of the screen (desktop).
  if ( (lDesktopY = WinQuerySysValue( HWND_DESKTOP, SV_CYSCREEN )) == 0 )
    lDesktopY = rcl.yTop;
  //  yBottom = suggested y coordinate, unless that would cause clipping
  rcl.yBottom = ( (pptlPosition->y + rcl.yTop) > lDesktopY )
                 ? lDesktopY - rcl.yTop : pptlPosition->y;
  
  // Set frame window position and size
  
  WinSetWindowPos(hwndFrame,
                  NULL,                      // relative wnd placement order
                  (SHORT) rcl.xLeft,         // window x coordinate
                  (SHORT) rcl.yBottom,       // window y coordindate
                  (SHORT) rcl.xRight,        // window x size
                  (SHORT) rcl.yTop,          // window y size
                  SWP_MOVE | SWP_SIZE);      // change window x,y position
                                             // change window size
  // Write the new position into OS2.INI so it can be used as the
  // date/time display's initial position when TIMELINE is next started
  // without parameters.  Double use a convenient POINTL structure.
  // The information is copied into a structure that is guaranteed to
  // be continuous.
  aptl[TXTBOX_TOPLEFT].x = rcl.xLeft;
  aptl[TXTBOX_TOPLEFT].y = rcl.yBottom;        
  WinWriteProfileData(hab,                          // handle to anchor block
                      szAppName,                    // name of this appl
                      szKeyName1,                   // keyword for info
                      &aptl[TXTBOX_TOPLEFT],        // info to write (save)
                      sizeof aptl[TXTBOX_TOPLEFT]); // bytes to write
}



VOID UpdateTime( HWND hwnd )
{
  CHAR   *szTime;
  CHAR    szFormattedTime[40];
  RECTL   rcl;                          // rectangle structure
  time_t  lTime;                        // long
  
  // Get ASCII time and date string and format it.  Padding with blanks
  // assures that shorter date/time strings (due to proportional fonts) will
  // always overwrite longer strings.
  
  time( &lTime );                                    // seconds since 1970
  szTime = ctime(&lTime);                            // convert to date/time
  strcpy( szFormattedTime, "   " );                  // start with blanks
  strcat( szFormattedTime, szTime );                 // add date/time
  szFormattedTime[strlen(szFormattedTime)-1] = '\0'; // get rid of line feed
  strcat( szFormattedTime, "   " );                  // add trailing blanks 
  
  // Display string in client window using the current font and foreground
  // and background colors.
  
  WinQueryWindowRect(hwnd,
                     &rcl);             // set to window coordinates
  GpiSetBackMix(hps,
                BM_OVERPAINT);          // new background overwrites
  WinDrawText(hps,                      // handle to presentation space
              -1,                       // string is zero terminated
              szFormattedTime,          // string to be drawn
              &rcl,                     // rectangle in which string is drawn
              CLR_NEUTRAL,              // default foreground (text) color
              CLR_BACKGROUND,           // default background color
              DT_CENTER | DT_VCENTER);  // center text horizontally
                                        // center text vertically
}


BOOL MoveWindow( HWND hwndFrame,PPOINTL pptlPosition, BOOL fKeyboard )
{
  static TRACKINFO tiFrame = {          // tracking information structure
                              1,        // cxBorder (border width)
                              1,        // cyBorder (border height)
                              1,        // cxGrid (horizontal tracking unit)
                              1,        // cyGrid (vertical tracking unit)
                              1,        // cxKeyboard (pixel increments for
                              1,        // cyKeyboard  keyboard interface)
                                        // Change the preceding two params
                                        // to something like 8 and 8 if you
                                        // want to speed up the movement of
                                        // the tracking rectangle when using
                                        // the arrow keys
                              {         // rclTrack (starting and ending)
                               0,0,0,0  // xLeft, yBottom, xRight, yTop
                              },
                              {         // rclBoundary (bounds of rectangle)
                               0,0,0,0
                              },
                              {         // ptlMinTrackSize
                               0,0       // x, y
                              },
                              {         // ptlMaxTrackSize
                               0,0       // x, y
                              },
                              0,        // fs (tracking options)
                              0,        // cxLeft   (ignored unless
                              0,        // cyBottom  TF_PARTINBOUNDARY is
                              0,        // cxRight   set)
                              0         // cyTop
         };

  WinQueryWindowRect(hwndFrame,                    // get size of frame
                     &tiFrame.rclTrack);
  // Map the relative coordinates of the position of the frame into
  // desktop coordinates
  WinMapWindowPoints(hwndFrame,
                     HWND_DESKTOP,
                     (PPOINTL) &tiFrame.rclTrack,  // coordinates
                     2L);                          // for a rectangle
  // Allow tracking frame to move anywhere on the desktop.
  tiFrame.fs = TF_MOVE | TF_ALLINBOUNDARY |
               (fKeyboard ? TF_SETPOINTERPOS : 0);

  // Want the minimum and maximum size of the tracking box to always
  // be the same size as the frame.
  tiFrame.ptlMaxTrackSize.x =
  tiFrame.ptlMinTrackSize.x = tiFrame.rclTrack.xRight -
                              tiFrame.rclTrack.xLeft;
  tiFrame.ptlMaxTrackSize.y =
  tiFrame.ptlMinTrackSize.y = tiFrame.rclTrack.yTop -
                              tiFrame.rclTrack.yBottom;

  // Confine the movement of the tracking rectangle to the desktop.
  tiFrame.rclBoundary.xRight = WinQuerySysValue( HWND_DESKTOP,SV_CXSCREEN );
  tiFrame.rclBoundary.yTop   = WinQuerySysValue( HWND_DESKTOP,SV_CYSCREEN );

  // Have PM draw the tracking rectangle and follow mouse movements.
  // Return FALSE if the move was canceled by pressing the ESC key.
  if ( WinTrackRect(HWND_DESKTOP,               // track over entire desktop
                    NULL,
                    &tiFrame) ) {               // location of tracking info
    pptlPosition->x = tiFrame.rclTrack.xLeft;
    pptlPosition->y = tiFrame.rclTrack.yBottom;
    return TRUE;
  }
  return FALSE;
}



MRESULT EXPENTRY ClientWndProc( HWND hwnd, USHORT msg, MPARAM mp1, MPARAM mp2 )
{
         HDC     hdc;                   // handle to device context of hwnd
  static HWND    hwndFrame;             // handle to window
  static struct {
                 CHAR *szColorKeyword;  // keyword
                 LONG  ColorIndex;      // corresponding color index
         } Color[MAX_COLORS] = {
                                {"BACKGROUND", CLR_BACKGROUND},
                                {"BLUE",       CLR_BLUE},
                                {"RED",        CLR_RED},
                                {"PINK",       CLR_PINK},
                                {"GREEN",      CLR_GREEN},
                                {"CYAN",       CLR_CYAN},
                                {"YELLOW",     CLR_YELLOW},
                                {"NEUTRAL",    CLR_NEUTRAL},
                                {"DARKGRAY",   CLR_DARKGRAY},
                                {"DARKBLUE",   CLR_DARKBLUE},
                                {"DARKRED",    CLR_DARKRED},
                                {"DARKPINK",   CLR_DARKPINK},
                                {"DARKGREEN",  CLR_DARKGREEN},
                                {"DARKCYAN",   CLR_DARKCYAN},
                                {"BROWN",      CLR_BROWN},
                                {"PALEGRAY",   CLR_PALEGRAY}
           };
  static LONG   alColorTable[MAX_COLORS*2];  // this array is actually pairs
                                             // of LONG's (index, RGB)
  static INT    iColor = 0;      // index into alColorTable for background
                                 // color of date/time string
  static INT    iIndex = 0;      // temporary
  static SIZEL  sizel = {0L,0L}; // want the default presentation page size
  static POINTL ptlPosition = { 0L, 0L};  // initial x and y coordinates of
                                          // the lower left corner of the
                                          // date/time display
  static BOOL   FakeButton1Down = FALSE;  // TRUE if drag of date/time display
                                          // is initiated from the keyboard
  static BOOL   ProcessArrows   = TRUE;   // FALSE if WM_CHAR up or down arrows
                                          // should be ignored (because they
                                          // are left over from moving the
                                          // date/time display by using ALT+F7
                                          // and then using the arrow keys)
  static BOOL   CycleColorsBack = FALSE;  // TRUE if the up arrow key is
                                          // being used to cycle display
                                          // colors backward
         USHORT usDataSize;                // short temporary

  switch ( msg ) {

    // Received when window is first created, before it is displayed.  In
    // particular, entry is here before the WinCreateStdWindow() in main()
    // has a chance to return a value.  Therefore cannot have one global
    // hwndFrame that is initialized by main().
    case WM_CREATE:
         hwndFrame = WinQueryWindow(hwnd,         // want handle to the parent
                                    QW_PARENT,    // (ie, to the frame) of
                                                  // this window
                                    FALSE);       // do not lock parent
         
         // Create a presentation space for the client that everyone will use
         // for drawing.  Having one space allows us to mess around with the
         // colors and not have them go away.
         hdc = WinOpenWindowDC ( hwnd );   // get this window's device context
         hps = GpiCreatePS(hab,            // handle to anchor block
                           hdc,            // handle to device context
                           &sizel,         // presentation page size
                           PU_PELS      |  // pel (pixel) coordinates
                           GPIF_DEFAULT |  // default coordinate format
                           GPIT_NORMAL  |  // normal (not micro) pres space
                           GPIA_ASSOC);    // association with hdc required

         // Get the colors to cycle through when mouse Button 2 is clicked.
         GpiQueryLogColorTable(hps,                  // presentation space
                               (ULONG) LCOLOPT_INDEX,// return index, RGB pairs
                               0L,                   // starting index
                               (LONG) MAX_COLORS*2,  // number of pairs*2
                               alColorTable);        // array to hold pairs

         // See if this was started with any parameters.  If so, use them.
         // If not, check OS2.INI for parameters. In any case, check for
         // sanity.
         if ( cArg <= 1 ) {
           // TIMELINE was not started with parameters.
           usDataSize = sizeof ptlPosition;
           if ( !WinQueryProfileData(hab,           //handle to anchor block
                                     szAppName,     //application name
                                     szKeyName1,    //keyword for data
                                     &ptlPosition,  //array to receive data
                                     &usDataSize) )  //bytes of data
              ptlPosition.x = ptlPosition.y = 0L;
           usDataSize = sizeof iColor;
           if ( !WinQueryProfileData( hab, szAppName, szKeyName2,
                                      &iColor, &usDataSize ) )
             iColor = CLR_BACKGROUND;
         }

         if ( cArg >= 3 ) {

           // The first parameter is the x coordinate.  Defaulted to 0
           // by its declaration.
           ptlPosition.x = atol( *(ppchArg+1) );

           // The second parameter is the y coordinate.  Defaulted to 0
           // by its declaration.
           ptlPosition.y = atol( *(ppchArg+2) );
         }

         if ( cArg >= 4 ) {
         // The third parameter is the text background color.  If it does
         // not match with one of the known colors, use CLR_BACKGROUND.

           // Match keyword against known strings of known colors
           while ( strcmpi( Color[iIndex].szColorKeyword, *(ppchArg+3) ) ) {
             if ( iIndex++ == MAX_COLORS-1 ) {
               iIndex = 0;
               break;
             }
           }
           // Convert index of string into index of desired color
           for ( iColor = 0; iColor < MAX_COLORS*2; iColor += 2)
             if ( Color[iIndex].ColorIndex == alColorTable[iColor] ) break;
           if ( iColor >= MAX_COLORS*2 ) iColor = CLR_BACKGROUND;
         }

         // Sanity check parameters.  If x or y position is less than 0,
         // make it 0.  If the color index doesn't fit in the logical
         // color table, make it CLR_BACKGROUND.  SizeWindow() will make
         // sure that the date/time display is initially fully visible
         // on the desktop (that is, that the x and y coordinate parameters
         // end up making sense).
         if ( ptlPosition.x < 0 )  ptlPosition.x = 0;
         if ( ptlPosition.y < 0 )  ptlPosition.y = 0;
         if ( iColor >= MAX_COLORS*2 || iColor < 0 ) iColor = CLR_BACKGROUND;

         // Set background color to choice.
         GpiCreateLogColorTable(hps,             // presentation space effected
                                (ULONG) 0,       // no options
                                LCOLF_CONSECRGB, // last param is array of
                                                 // consecutive RGB values
                                CLR_BACKGROUND,  // color table index of
                                                 // first RGB value
                                1L,              // number of values in
                                                 // last parameter
                                &alColorTable[iColor+1]);

         // Remember background color in case TIMELINE is started without
         // parameters.
         WinWriteProfileData(hab,                 // handle to anchor block
                             szAppName,           // name of this application
                             szKeyName2,          // keyword for info
                             &iColor,             // info to write (save)
                             sizeof iColor);      // number of bytes to write

         // Determine size and location of date/time display.
         SizeWindow( hwndFrame, &ptlPosition );      // suggested x,y coords

         WinStartTimer( hab, hwnd, ID_TIMER, 1000 ); // one second per message
         return 0;

    // Received when the timer goes off
    case WM_TIMER:
         // update the date/time display with current information
         UpdateTime( hwnd );
         return 0;

    // Received when the PM thinks the window needs repainting
    case WM_PAINT:
         WinBeginPaint(hwnd,               // window in which to draw
                       hps,                // use the existing present space
                       NULL);              // repainting not required
         GpiErase( hps );                  // erase invalid area of window
                                           // using zeroth color index value

         // update the date/time display with current information
         UpdateTime( hwnd );
         
         WinEndPaint( hps );
         return 0;

    // Received when mouse Button 1 is held down.  Make a tracking rectangle
    // follow the mouse movements until the mouse is released, then redraw
    // the date/time display at the new position.
    case WM_BUTTON1DOWN:
         WinSetFocus( HWND_DESKTOP, hwnd );       // make this window active
                                                  // and have input focus
         if ( MoveWindow( hwndFrame, &ptlPosition, FakeButton1Down ) ) {
           // Draw the data/time display where the tracking rectangle stopped
           WinSetWindowPos(hwndFrame,
                           HWND_TOP,              // place on top of other wnds
                           (SHORT) ptlPosition.x, // use new track frame x,y
                           (SHORT) ptlPosition.y, // for new position
                           0,                     // ignored
                           0,                     // ignored
                           SWP_MOVE);             // window move request
           // Write the new position into OS2.INI so it can be used as the
           // date/time display's initial position when TIMELINE is next
           // started without parameters.
           WinWriteProfileData(hab,                 // handle to anchor block
                               szAppName,           // this application's name
                               szKeyName1,          // keyword for info
                               &ptlPosition,        // info to save (write)
                               sizeof ptlPosition); // number of bytes to save
         }
         FakeButton1Down = FALSE;
         // Put marker into queue that WM_CHAR processes to indicate end
         // of arrow keystrokes that should be ignored
         WinPostMsg( hwnd,
                     WM_CHAR,
                     MPFROMSH2CH((KC_KEYUP|KC_VIRTUALKEY), 0, 0),
                     MPFROM2SHORT(0,VK_BUTTON3) );
         return 0;

    // Received when mouse Button 2 is double clicked.  Treat as a
    // WM_BUTTON2DOWN because the user is probably trying to cycle the
    // mouse colors too fast.  Note that the WM_BUTTON2DOWN code must
    // directly follow this case.
    case WM_BUTTON2DBLCLK:

    // Received when mouse Button 2 is held down.  Change to the next text
    // background color.
    //
    // Its interesting to note that Button 2 cannot be made to act just
    // like Button 1 because WinTrackRect only seems to work for Button 1.
    case WM_BUTTON2DOWN:
         WinSetFocus( HWND_DESKTOP, hwnd );      // make sure of input focus
         // if necessary, wrap index to start of color table
         if ( CycleColorsBack ) {
           CycleColorsBack = FALSE;
           iColor = ( --iColor <= 0 ) ? MAX_COLORS*2-2 : --iColor;
         }
         else iColor = ( ++iColor == MAX_COLORS*2-1 ) ? 0 : ++iColor;
         // set background color to new choice
         GpiCreateLogColorTable(hps,             // presentation space effected
                                (ULONG) 0,       // no options
                                LCOLF_CONSECRGB, // last parameter is array
                                                 // of consecutive RGB values
                                CLR_BACKGROUND,  // color table index of
                                                 // first RGB value
                                1L,              // number of values in last
                                                 // parameter
                                &alColorTable[iColor+1]);
         WinInvalidateRegion(hwnd,    // window to force repaint on
                             NULL,    // whole window is invalid
                             FALSE);  // don't invalidate children windows
         // Write the color index into OS2.INI so it can be used as the
         // date/time display's initial background color when TIMELINE is
         // next started without parameters.
         WinWriteProfileData(hab,                 // handle to anchor block
                             szAppName,           // name of this application
                             szKeyName2,          // keyword for info
                             &iColor,             // info to write (save)
                             sizeof iColor);      // number of bytes to write
         return 0;

    //Received when mouse Button 1 is double clicked.  Terminate.
    case WM_BUTTON1DBLCLK:
         WinPostMsg( hwnd, WM_QUIT, NULL, NULL );  // see main()
         return 0;

    // Received when F1 is typed and handled through the ACCELTABLE.
    // Produce a simple help message by a not so simple method
    // (that is, use a real dialog box rather than something like
    // WinMessageBox).
    case WM_HELP:
         switch ( COMMANDMSG(&msg)->source ) {

           // The following three cases should never occur for TIMELINE
           case CMDSRC_PUSHBUTTON:
           case CMDSRC_MENU:
           case CMDSRC_OTHER:

           case CMDSRC_ACCELERATOR:
                WinDlgBox(HWND_DESKTOP,      // parent window (ie, allow
                                             // dialog box anywhere on dsktop)
                          hwndFrame,         // owner window
                          HelpDlgProc,       // address of dialog procedure
                          NULL,              // dialog box in .exe file
                          timeline_help,     // dialog identifier in .exe file
                          NULL);             // dialog procedure data
                WinInvalidateRegion(hwnd,    // window to force repaint on
                                    NULL,    // whole window is invalid
                                    FALSE);  // don't invalidate children wnds
                return 0;

           default:
                return 0;
         }

    // Received when a character is typed and is not handled by something
    // like the ACCELTABLE.  Ignore the character unless it is
    //
    // Alt+F7      then treat as if Mouse Button 1 is being held down (that
    //             is, get ready to drag the display
    // Alt+F4      then terminate
    // F3          then terminate
    // UP ARROW    then cycle display background color backward
    // DOWN ARROW  then cycle display background color forward
    case WM_CHAR:
         switch ( CHARMSG(&msg)->vkey  ) {

           // Track cursor movement by keyboard arrows. Alt+F7 is treated
           // the same way as BUTTON1DOWN
           case VK_F7:
                if ( (CHARMSG(&msg)->fs & VKEY_MASK) ==
                     (KC_KEYUP | KC_ALT | KC_VIRTUALKEY) ) {
                  FakeButton1Down = TRUE;
                  ProcessArrows = FALSE;
                  WinPostMsg( hwnd, WM_BUTTON1DOWN, NULL, NULL );
                }
                return 0;

           // Terminate/Close
           case VK_F4:
                if ( (CHARMSG(&msg)->fs & VKEY_MASK) == 
                     (KC_KEYUP | KC_ALT | KC_VIRTUALKEY) ) {
                  WinPostMsg( hwnd, WM_QUIT, NULL, NULL );  // see main()
                }
                return 0;

           // Terminate/Close
           case VK_F3:
                if ( (CHARMSG(&msg)->fs & VKEY_MASK) ==
                     (KC_KEYUP | KC_VIRTUALKEY) ) {
                  WinPostMsg( hwnd, WM_QUIT, NULL, NULL );  // see main()
                }
                return 0;

           // Up arrow decrements color index
           case VK_UP:
                if ( ProcessArrows ) {
                  if ( (CHARMSG(&msg)->fs & VKEY_MASK) ==
                       (KC_KEYUP | KC_VIRTUALKEY) ) {
                    CycleColorsBack = TRUE;
                    WinPostMsg( hwnd, WM_BUTTON2DOWN, NULL, NULL );
                  }
                }
                return 0;

           // Down arrow increments color index (identical to clicking
           // mouse button 2)
           case VK_DOWN:
                if ( ProcessArrows ) {
                  if ( (CHARMSG(&msg)->fs & VKEY_MASK) ==
                       (KC_KEYUP | KC_VIRTUALKEY) ) {
                    WinPostMsg( hwnd, WM_BUTTON2DOWN, NULL, NULL );
                  }
                }
                return 0;

           // Received fake marker character that indicates WinTrackRect
           // is done and therefore arrow keys should be processed again.
           case VK_BUTTON3:
                if ( (CHARMSG(&msg)->fs & VKEY_MASK) ==
                     (KC_KEYUP | KC_VIRTUALKEY) ) {
                  ProcessArrows = TRUE;
                }
                return 0;

           default:
                return 0;
           }

    // Received when main() has asked this window to go away
    case WM_DESTROY:
         WinStopTimer( hab, hwnd, ID_TIMER );
         GpiDestroyPS( hps );
         return 0;
    }

  // If the message received isn't handled in the immediately preceding
  // switch statement, fall into the next statement, which passes the
  // message to the default window procedure.
  return WinDefWindowProc( hwnd, msg, mp1, mp2 );

}



MRESULT EXPENTRY HelpDlgProc( HWND hwndDlg, USHORT msg, MPARAM mp1, MPARAM mp2 )
{

  // Received an event message for the dialog box.  Process it below or
  // pass it to the default dialog procedure. 
  switch ( msg ) {

    // Received when a control in the dialog box has something to say.
    // A command is ignored if it is not handled below.
    case WM_COMMAND:
      switch ( COMMANDMSG(&msg)->cmd ) {

        // Normally a dialog box has an "Enter" button, which when pushed
        // results in transfer of control to here. As the help dialog
        // box has no Enter button, control should never get to here.
        case DID_OK:
             return 0;

        // Typed ESC or pushed the ESC=Cancel button or typed Enter
        // (because the ESC=Cancel button is the default)
        case DID_CANCEL:
             WinDismissDlg( hwndDlg, TRUE );  // remove the dialog box
             return 0;

        // Ignore other commands
        default:
             return 0;
      }

    // Received an event message that this procedure does not process.
    // Pass the message to the default dialog procedure.
    default:
       return WinDefDlgProc( hwndDlg, msg, mp1, mp2 );
  }

  return 0;
}
