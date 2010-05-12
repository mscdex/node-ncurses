/* Constants ==================================================================== */
var keys = {};
var colors = {};
var attrs = {};

keys['BREAK'] = 0x101;    /* break key */
keys['DOWN'] = 0x102;    /* down arrow */
keys['UP'] = 0x103;    /* up arrow */
keys['LEFT'] = 0x104;    /* left arrow */
keys['RIGHT'] = 0x105;    /* right arrow*/
keys['HOME'] = 0x106;    /* home key */
keys['BACKSPACE'] = 0x107;    /* Backspace */

/* Function keys */
keys['F0'] = 0x108;
for (var i=1; i<=12; i++)
	keys['F'+i] = keys['F0']+i;

keys['DL'] = 0x148;    /* Delete Line */
keys['IL'] = 0x149;    /* Insert Line*/
keys['DC'] = 0x14A;    /* Delete Character */
keys['IC'] = 0x14B;    /* Insert Character */
keys['EIC'] = 0x14C;    /* Exit Insert Char mode */
keys['CLEAR'] = 0x14D;    /* Clear screen */
keys['EOS'] = 0x14E;    /* Clear to end of screen */
keys['EOL'] = 0x14F;    /* Clear to end of line */
keys['SF'] = 0x150;    /* Scroll one line forward */
keys['SR'] = 0x151;    /* Scroll one line back */
keys['NPAGE'] = 0x152;    /* Next Page */
keys['PPAGE'] = 0x153;    /* Prev Page */
keys['STAB'] = 0x154;    /* Set Tab */
keys['CTAB'] = 0x155;    /* Clear Tab */
keys['CATAB'] = 0x156;    /* Clear All Tabs */
keys['ENTER'] = 0x157;    /* Enter or Send */
keys['SRESET'] = 0x158;    /* Soft Reset */
keys['RESET'] = 0x159;    /* Hard Reset */
keys['PRINT'] = 0x15A;    /* Print */
keys['LL'] = 0x15B;    /* Home Down */

/*
 * "Keypad" keys arranged like this:
 *
 *  A1   up  A3
 * left  B2 right
 *  C1  down C3
 *
 */
keys['A1'] = 0x15C;    /* Keypad upper left */
keys['A3'] = 0x15D;    /* Keypad upper right */
keys['B2'] = 0x15E;    /* Keypad centre key */
keys['C1'] = 0x15F;    /* Keypad lower left */
keys['C3'] = 0x160;    /* Keypad lower right */

keys['BTAB'] = 0x161;    /* Back Tab */
keys['BEG'] = 0x162;    /* Begin key */
keys['CANCEL'] = 0x163;    /* Cancel key */
keys['CLOSE'] = 0x164;    /* Close Key */
keys['COMMAND'] = 0x165;    /* Command Key */
keys['COPY'] = 0x166;    /* Copy key */
keys['CREATE'] = 0x167;    /* Create key */
keys['END'] = 0x168;    /* End key */
keys['EXIT'] = 0x169;    /* Exit key */
keys['FIND'] = 0x16A;    /* Find key */
keys['HELP'] = 0x16B;    /* Help key */
keys['MARK'] = 0x16C;    /* Mark key */
keys['MESSAGE'] = 0x16D;    /* Message key */
keys['MOVE'] = 0x16E;    /* Move key */
keys['NEXT'] = 0x16F;    /* Next Object key */
keys['OPEN'] = 0x170;    /* Open key */
keys['OPTIONS'] = 0x171;    /* Options key */
keys['PREVIOUS'] = 0x172;    /* Previous Object key */
keys['REDO'] = 0x173;    /* Redo key */
keys['REFERENCE'] = 0x174;    /* Ref Key */
keys['REFRESH'] = 0x175;    /* Refresh key */
keys['REPLACE'] = 0x176;    /* Replace key */
keys['RESTART'] = 0x177;    /* Restart key */
keys['RESUME'] = 0x178;    /* Resume key */
keys['SAVE'] = 0x179;    /* Save key */
keys['SBEG'] = 0x17A;    /* Shift begin key */
keys['SCANCEL'] = 0x17B;    /* Shift Cancel key */
keys['SCOMMAND'] = 0x17C;    /* Shift Command key */
keys['SCOPY'] = 0x17D;    /* Shift Copy key */
keys['SCREATE'] = 0x17E;    /* Shift Create key */
keys['SDC'] = 0x17F;    /* Shift Delete Character */
keys['SDL'] = 0x180;    /* Shift Delete Line */
keys['SELECT'] = 0x181;    /* Select key */
keys['SEND'] = 0x182;    /* Send key */
keys['SEOL'] = 0x183;    /* Shift Clear Line key */
keys['SEXIT'] = 0x184;    /* Shift Exit key */
keys['SFIND'] = 0x185;    /* Shift Find key */
keys['SHELP'] = 0x186;    /* Shift Help key */
keys['SHOME'] = 0x187;    /* Shift Home key */
keys['SIC'] = 0x188;    /* Shift Input key */
keys['SLEFT'] = 0x189;    /* Shift Left Arrow key */
keys['SMESSAGE'] = 0x18A;    /* Shift Message key */
keys['SMOVE'] = 0x18B;    /* Shift Move key */
keys['SNEXT'] = 0x18C;    /* Shift Next key */
keys['SOPTIONS'] = 0x18D;    /* Shift Options key */
keys['SPREVIOUS'] = 0x18E;    /* Shift Previous key */
keys['SPRINT'] = 0x18F;    /* Shift Print key */
keys['SREDO'] = 0x190;    /* Shift Redo key */
keys['SREPLACE'] = 0x191;    /* Shift Replace key */
keys['SRIGHT'] = 0x192;    /* Shift Right Arrow key */
keys['SRSUME'] = 0x193;    /* Shift Resume key */
keys['SSAVE'] = 0x194;    /* Shift Save key */
keys['SSUSPEND'] = 0x195;    /* Shift Suspend key */
keys['SUNDO'] = 0x196;    /* Shift Undo key */
keys['SUSPEND'] = 0x197;    /* Suspend key */
keys['UNDO'] = 0x198;    /* Undo key */
keys['MOUSE'] = 0x199;    /* Mouse event has occurred */
keys['RESIZE'] = 0x200;    /* Resize event has occurred */

colors['BLACK'] = 0x00;
colors['RED'] = 0x01;
colors['GREEN'] = 0x02;
colors['YELLOW'] = 0x03;
colors['BLUE'] = 0x04;
colors['MAGENTA'] = 0x05;
colors['CYAN'] = 0x06;
colors['WHITE'] = 0x07;

attrs['CHARTEXT'] = 0x000000ff;	/* bits for 8-bit characters */
attrs['NORMAL'] = 0x00000000;	/* Added characters are normal. */
attrs['STANDOUT'] = 0x00000100;	/* Added characters are standout. */
attrs['UNDERSCORE'] = 0x00000200;	/* Added characters are underscored. */
attrs['REVERSE'] = 0x00000400;	/* Added characters are reverse video. */
attrs['BLINK'] = 0x00000800;	/* Added characters are blinking. */
attrs['DIM'] = 0x00001000;	/* Added characters are dim. */
attrs['BOLD'] = 0x00002000;	/* Added characters are bold. */
attrs['BLANK'] = 0x00004000;	/* Added characters are blanked. */
attrs['PROTECT'] = 0x00008000;	/* Added characters are protected. */
attrs['ALTCHARSET'] = 0x00010000;	/* Added characters are ACS */
attrs['COLOR'] = 0x03fe0000;	/* Color bits */
attrs['ATTRIBUTES'] = 0x03ffff00;	/* All 8-bit attribute bits */

exports.keys = keys;
exports.colors = colors;
exports.attrs = attrs;
/* ============================================================================= */