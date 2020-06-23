//
// explorerRsc.h : Resource ID definitions
//                 for axxPac explorer
//

#include "explorer.h"

// The form
#define frm_Main      1000
#define scl_Main      1001

#define DispPalm      1002
#define DispCard      1003

#define SelInfo       1005
#define SelCopy       1006
#define SelMove       1007
#define SelDelete     1008

#define SelPopup      1005
#define SelList       1006

#define IconInfo      1015
#define IconCopy      1016
#define IconMove      1017
#define IconDelete    1018

#define mnu_Main      1100
#define menuDownload  1101
#define menuUpload    1102
#define menuAbout     1103
#define menuNewDir    1104
#define menuFormat    1105
#define menuPref      1106
#define menuCard      1107
#define menuPlug      1108
#define menuBackup    1109
#define menuRestore   1110
#define menuOptions   1111
#define menuMemoExp   1112

#define frm_DbInfo    1200
#define DbName        1201
#define DbSize        1202
#define DbDate        1203
#define DbTime        1204
#define DbOk          1205
#define DbRsrc        1206
#define DbRO          1207
#define DbVersion     1208
#define DbCreator     1209
#define DbType        1210
#define DbRec         1211
#define DbEntries     1212

// the icons
#define IconError     1300
#define IconInserted  1301
#define IconBusy      1302
#define IconFolder    1303
#define IconSync      1304
#define IconText      1305
#define IconDB        1306

// the new directory dialog
#define frm_NewDir    1400
#define NewDirName    1401
#define NewDirOK      1402
#define NewDirCancel  1403
#define NewDirHelp    1404

// the file info dialog
#define frm_FileInfo   1450
#define FileInfoName   1451
#define FileInfoOK     1452
#define FileInfoCancel 1453
#define FileInfoSize   1454
#define FileInfoDir    1455
#define FileInfoRO     1456
#define FileInfoDate   1457
#define FileInfoTime   1458
#define FileInfoPlugin 1459
#define FileInfoMove   1460
#define FileInfoBeam   1461

// the text view dialog
#define frm_View       1500
#define ViewField      1501
#define ViewScrollbar  1502
#define ViewOK         1503
#define ViewInst       1504

// the auto backup dialog
#define frm_Auto       1550

// the about dialog
#define frm_About      1600
#define AboutLogo      1601
#define AboutOK        1602

// the plugin config dialog
#define frm_PlugCfg    1650
#define PlugCfgOk      1651
#define PlugCfg1       1660
#define PlugCfg2       1661
#define PlugCfg3       1662
#define PlugCfg4       1663
#define PlugCfg5       1664
#define PlugCfg6       1665
#define PlugCfg7       1666
#define PlugCfg8       1667
#define PlugCfg1Name   1670
#define PlugCfg2Name   1671
#define PlugCfg3Name   1672
#define PlugCfg4Name   1673
#define PlugCfg5Name   1674
#define PlugCfg6Name   1675
#define PlugCfg7Name   1676
#define PlugCfg8Name   1677

// the preferences dialog
#define frm_Pref       1700
#define PrefHelp       1701
#define PrefOK         1702
#define PrefCancel     1703
#define PrefOverwrite  1704
#define PrefDelete     1705
#define PrefROM        1706
#define PrefMode       1707
#define PrefSortSel    1708
#define PrefSortLst    1709

// the backup options dialog
#define frm_Options    1750
#define OptionsOK      1752
#define OptionsCancel  1753
#define OptionsECreat0 1754
#define OptionsCreat0  1755
#define OptionsEType0  1756
#define OptionsType0   1757
#define OptionsECreat1 1758
#define OptionsCreat1  1759
#define OptionsEType1  1760
#define OptionsType1   1761
#define OptionsECreat2 1762
#define OptionsCreat2  1763
#define OptionsEType2  1764
#define OptionsType2   1765
#define OptionsECreat3 1766
#define OptionsCreat3  1767
#define OptionsEType3  1768
#define OptionsType3   1769
#define OptionsHelp    1770
#define OptionsAuto    1772
#define OptionsTime    1773

// the card info dialog
#define frm_Card       1800
#define CardOK         1801
#define CardNone       1803
#define CardLblVendor  1804
#define CardVendor     1805
#define CardLblType    1806
#define CardType       1807
#define CardLblSize    1808
#define CardSize       1809
#define CardBInfo      1810
#define CardTZone      1811
#define CardTFree      1812
#define CardTUsed      1813
#define CardTUnus      1814
#define CardScl        1815
#define CardLast       1815

#define CardLblLib     1832
#define CardLblVer     1833
#define CardLibVer     1834

// the memo import stuff
#define frm_MemoSel    1850
#define MemoScrollbar  1851
#define MemoName0      1852
#define MemoName1      1853
#define MemoName2      1854
#define MemoName3      1855
#define MemoName4      1856
#define MemoName5      1857
#define MemoName6      1858
#define MemoName7      1859
#define MemoName8      1860
#define MemoName9      1861
#define MemoCancel     1862

// lots of error/warning strings
#define StrNoPrcPdb   1907
#define StrNoPalmDB   1908
#define StrExtRec     1909
#define StrSortInfo   1910
//#define StrCreateDB   1911
#define StrFindDB     1912
#define StrOpenDB     1913
//#define StrMemRecHdr  1914
//#define StrNoData     1915
#define StrMemBuf     1916
#define StrMemRec     1917
//#define StrFileOpen   1918
#define StrDeleteDB   1919
#define StrLstOOMDir  1946
//#define StrEmptyCath  1947
//#define StrAppInfOOM  1948
//#define StrAppInfRd   1949
//#define StrAppInfBuf  1950
//#define StrWrRscOOM   1951
//#define StrWrRecOOM   1952
//#define StrRdRec      1953
//#define StrWrRscHOOM  1954
//#define StrWrRecHOOM  1955
#define StrLibVer     1956
#define StrFindFile   1957
#define StrNoMemo     1958

#define alt_err       2000
#define alt_err2      2001
#define alt_copy2memo 2002
#define alt_overwrite 2003
#define alt_delete    2004
#define alt_format    2005
#define alt_noplug    2006
#define alt_backup    2007
#define alt_restore   2008
#define alt_delsub    2009
#define alt_sdfull    2010



