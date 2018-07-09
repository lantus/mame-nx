/*	After Burner Hardware
**	2xMC68000 + Z80
**	YM2151 + Custom PCM
**
**	AB Cop (Protected?)
**	After Burner I & II
**	GP Rider (Protected?)
**	Last Survivor (Protected?)
**	Line of Fire (Protected?)
**	Super Monaco GP (Protected)
**	Thunder Blade (Protected)
**	Thunder Blade Japan
*/

/*
Special thanks to:
- Thierry Lescot & Nao (Hardware Info)
- Mirko Buffoni (original MAME driver)
- Phil Stroffolino
- Andrew Prime
- Dave (www.finalburn.com) for sharing his understanding of the Afterburner Math Coprocessor
*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"
#include "cpu/i8039/i8039.h"
#include "system16.h"

/*****************************************************************************/
/* After Burner I (Japanese Version)
(c) 1987 SEGA

 Length  Method   Size  Ratio   Date    Time    CRC-32  Attr  Name
 ------  ------   ----- -----   ----    ----   -------- ----  ----
 131072  DeflatN  55764  58%  08-30-99  00:00  d8437d92 ---  EPR10949.BIN
 131072  DeflatN  54269  59%  08-30-99  00:00  64284761 ---  EPR10948.BIN
 131072  DeflatN  54081  59%  08-30-99  00:00  08838392 ---  EPR10947.BIN
 131072  DeflatN  55426  58%  08-30-99  00:00  d7d485f4 ---  EPR10946.BIN
 131072  DeflatN  65628  50%  08-30-99  00:00  df4d4c4f ---  EPR10945.BIN
 131072  DeflatN  63303  52%  08-30-99  00:00  17be8f67 ---  EPR10944.BIN
 131072  DeflatN  63735  52%  08-30-99  00:00  b98294dc ---  EPR10943.BIN
 131072  DeflatN  65627  50%  08-30-99  00:00  5ce10b8c ---  EPR10942.BIN
 131072  DeflatN  53625  60%  08-30-99  00:00  136ea264 ---  EPR10941.BIN
 131072  DeflatN  48191  64%  08-30-99  00:00  4d132c4e ---  EPR10940.BIN
 131072  DeflatN  13843  90%  08-30-99  00:00  7c01d40b ---  EPR10928.BIN
 131072  DeflatN   5843  96%  08-30-99  00:00  66d36757 ---  EPR10927.BIN

  65536  DeflatN  43470  34%  08-30-99  00:00  ed8bd632 ---  EPR10926.BIN
  65536  DeflatN  37414  43%  08-30-99  00:00  4ef048cc ---  EPR10925.BIN
  65536  DeflatN  24435  63%  08-30-99  00:00  50c15a6d ---  EPR10924.BIN
  65536  DeflatN  17210  74%  08-21-99  00:00  6888eb8f ---  EPR10923.BIN > snd prg
 ------          ------  ---                                  -------
1835008          721864  61%                                       16
*/

ROM_START( aburner )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr10940.bin",0,0x20000, CRC(4d132c4e) SHA1(007af52167c369177b86fc0f8b007ebceba2a30c) )
	ROM_LOAD16_BYTE( "epr10941.bin",1,0x20000, CRC(136ea264) SHA1(606ac67db53a6002ed1bd71287aed2e3e720cdf4) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "epr10926.bin",0x00000,0x10000, CRC(ed8bd632) SHA1(d5bbd5e257ebef8cfb3baf5fa530b189d9cddb57) )
	ROM_LOAD( "epr10925.bin",0x10000,0x10000, CRC(4ef048cc) SHA1(3b386b3bfa600f114dbc19796bb6864a88ff4562) )
	ROM_LOAD( "epr10924.bin",0x20000,0x10000, CRC(50c15a6d) SHA1(fc202cc583fc6804647abc884fdf332e72ea3100) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD( "10932.125",   0x20000*0x0, 0x20000, CRC(cc0821d6) SHA1(22e84419a585209bbda1466d2180504c316a9b7f) )
	ROM_LOAD( "10934.129",   0x20000*0x4, 0x20000, CRC(4a51b1fa) SHA1(2eed018a5a1e935bb72b6f440a794466a1397dc5) )
	ROM_LOAD( "10936.133",   0x20000*0x8, 0x20000, CRC(ada70d64) SHA1(ba6203b0fdb4c4998b7be5b446eb8354751d553a) )
	ROM_LOAD( "10938.102",   0x20000*0xc, 0x20000, CRC(e7675baf) SHA1(aa979319a44c0b18c462afb5ca9cdeed2292c76a) )
	ROM_LOAD( "10933.126",   0x20000*0x1, 0x20000, CRC(c8efb2c3) SHA1(ba31da93f929f2c457e60b2099d5a1ba6b5a9f48) )
	ROM_LOAD( "10935.130",   0x20000*0x5, 0x20000, CRC(c1e23521) SHA1(5e95f3b6ff9f4caca676eaa6c84f1200315218ea) )
	ROM_LOAD( "10937.134",   0x20000*0x9, 0x20000, CRC(f0199658) SHA1(cd67504fef53f637a3b1c723c4a04148f88028d2) )
	ROM_LOAD( "10939.103",   0x20000*0xd, 0x20000, CRC(a0d49480) SHA1(6c4234456bc09ae771beec284d7aa21ebe474f6f) )
	ROM_LOAD( "epr10942.bin",0x20000*0x2,0x20000, CRC(5ce10b8c) SHA1(c6c189143762b0ef473d5d31d66226820c5cf080) )
	ROM_LOAD( "epr10943.bin",0x20000*0x6,0x20000, CRC(b98294dc) SHA1(a4161af23f9a67b4ed81308c73e72e1797cce894) )
	ROM_LOAD( "epr10944.bin",0x20000*0xa,0x20000, CRC(17be8f67) SHA1(371f0dd1914a98695cb86f921fe8e82b49e69a4a) )
	ROM_LOAD( "epr10945.bin",0x20000*0xe,0x20000, CRC(df4d4c4f) SHA1(24075a6709869d9acf9082b6b4ad96bc6f8b1932) )
	ROM_LOAD( "epr10946.bin",0x20000*0x3,0x20000, CRC(d7d485f4) SHA1(d843aefb4d99e0dff8d62ee6bd0c3aa6aa6c941b) )
	ROM_LOAD( "epr10947.bin",0x20000*0x7,0x20000, CRC(08838392) SHA1(84f7ff3bff31c0738dead7bc00219ede834eb0e0) )
	ROM_LOAD( "epr10948.bin",0x20000*0xb,0x20000, CRC(64284761) SHA1(9594c671900f7f49d8fb965bc17b4380ce2c68d5) )
	ROM_LOAD( "epr10949.bin",0x20000*0xf,0x20000, CRC(d8437d92) SHA1(480291358c3d197645d7bd149bdfe5d41071d52d) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr10923.bin",0x00000, 0x10000, CRC(6888eb8f) SHA1(8f8fffb214842a5d356e33f5a97099bc6407384f) )

	ROM_REGION( 0x60000, REGION_SOUND1, 0 ) /* Sega PCM sound data */
	ROM_LOAD( "10931.11",    0x00000, 0x20000, CRC(9209068f) SHA1(01f3dda1c066d00080c55f2c86c506b6b2407f98) )
	ROM_LOAD( "10930.12",    0x20000, 0x20000, CRC(6493368b) SHA1(328aff19ff1d1344e9115f519d3962390c4e5ba4) )
	ROM_LOAD( "11102.13",    0x40000, 0x20000, CRC(6c07c78d) SHA1(3868b1824f43e4f2b4fbcd9274bfb3000c889d12) )

	ROM_REGION( 0x40000, REGION_CPU3, 0 ) /* 2nd 68000 code */
	ROM_LOAD16_BYTE( "epr10927.bin",0,0x20000, CRC(66d36757) SHA1(c7f6d653fb6bfd629bb62057010d41f3ccfccc4d) )
	ROM_LOAD16_BYTE( "epr10928.bin",1,0x20000, CRC(7c01d40b) SHA1(d95b4702a9c813db8bc24c8cd7e0933cbe54a573) )

	ROM_REGION( 0x10000, REGION_GFX3, 0 ) /* road gfx */
	ROM_LOAD( "10922.40", 0x000000, 0x10000, CRC(b49183d4) SHA1(71d87bfbce858049ccde9597ab15575b3cdba892) )
ROM_END

/*****************************************************************************/
// After Burner II
ROM_START( aburner2 )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "11107.58",  0x00000, 0x20000, CRC(6d87bab7) SHA1(ab34fe78f1f216037b3e3dca3e61f1b31c05cedf) )
	ROM_LOAD16_BYTE( "11108.104", 0x00001, 0x20000, CRC(202a3e1d) SHA1(cf2018bbad366de4b222eae35942636ca68aa581) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "11115.154", 0x00000, 0x10000, CRC(e8e32921) SHA1(30a96e6b514a475c778296228ba5b6fb96b211b0) )
	ROM_LOAD( "11114.153", 0x10000, 0x10000, CRC(2e97f633) SHA1(074125c106dd00785903b2e10cd7e28d5036eb60) )
	ROM_LOAD( "11113.152", 0x20000, 0x10000, CRC(36058c8c) SHA1(52befe6c6c53f10b6fd4971098abc8f8d3eef9d4) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD( "10932.125", 0x20000*0x0, 0x20000, CRC(cc0821d6) SHA1(22e84419a585209bbda1466d2180504c316a9b7f) )
	ROM_LOAD( "10933.126", 0x20000*0x1, 0x20000, CRC(c8efb2c3) SHA1(ba31da93f929f2c457e60b2099d5a1ba6b5a9f48) )
	ROM_LOAD( "11103.127", 0x20000*0x2, 0x20000, CRC(bdd60da2) SHA1(01673837c5ad84fa087728a05549ac01542ef4e9) )
	ROM_LOAD( "11116.128", 0x20000*0x3, 0x20000, CRC(49b4c1ba) SHA1(5419f49f091e386eead4ccf5e03f12769e278179) )
	ROM_LOAD( "10934.129", 0x20000*0x4, 0x20000, CRC(4a51b1fa) SHA1(2eed018a5a1e935bb72b6f440a794466a1397dc5) )
	ROM_LOAD( "10935.130", 0x20000*0x5, 0x20000, CRC(c1e23521) SHA1(5e95f3b6ff9f4caca676eaa6c84f1200315218ea) )
	ROM_LOAD( "11104.131", 0x20000*0x6, 0x20000, CRC(06a35fce) SHA1(c39ae02fc8246e883c4f4c320f668ce6ca9c845a) )
	ROM_LOAD( "11117.132", 0x20000*0x7, 0x20000, CRC(821fbb71) SHA1(be2366d7b4a3a2543ba5024f0e258f1bc43caec8) )
	ROM_LOAD( "10936.133", 0x20000*0x8, 0x20000, CRC(ada70d64) SHA1(ba6203b0fdb4c4998b7be5b446eb8354751d553a) )
	ROM_LOAD( "10937.134", 0x20000*0x9, 0x20000, CRC(f0199658) SHA1(cd67504fef53f637a3b1c723c4a04148f88028d2) )
	ROM_LOAD( "11105.135", 0x20000*0xa, 0x20000, CRC(027b0689) SHA1(c704c79faadb5e445fd3bd9281683b09831782d2) )
	ROM_LOAD( "11118.136", 0x20000*0xb, 0x20000, CRC(8f38540b) SHA1(1fdfb157d1aca96cb635bd3d64f94545eb88c133) )
	ROM_LOAD( "10938.102", 0x20000*0xc, 0x20000, CRC(e7675baf) SHA1(aa979319a44c0b18c462afb5ca9cdeed2292c76a) )
	ROM_LOAD( "10939.103", 0x20000*0xd, 0x20000, CRC(a0d49480) SHA1(6c4234456bc09ae771beec284d7aa21ebe474f6f) )
	ROM_LOAD( "11106.104", 0x20000*0xe, 0x20000, CRC(9e1fec09) SHA1(6cc47d86852b988bfcd64cb4ed7d832c683e3114) )
	ROM_LOAD( "11119.105", 0x20000*0xf, 0x20000, CRC(d0343a8e) SHA1(8c0c0addb6dfd0ea04c3900a9f7f7c731ca6e9ea) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "11112.17",    0x00000, 0x10000, CRC(d777fc6d) SHA1(46ce1c3875437044c0a172960d560d6acd6eaa92) )

	ROM_REGION( 0x60000, REGION_SOUND1, 0 ) /* Sega PCM sound data */
	ROM_LOAD( "10931.11",    0x00000, 0x20000, CRC(9209068f) SHA1(01f3dda1c066d00080c55f2c86c506b6b2407f98) )
	ROM_LOAD( "10930.12",    0x20000, 0x20000, CRC(6493368b) SHA1(328aff19ff1d1344e9115f519d3962390c4e5ba4) )
	ROM_LOAD( "11102.13",    0x40000, 0x20000, CRC(6c07c78d) SHA1(3868b1824f43e4f2b4fbcd9274bfb3000c889d12) )

	ROM_REGION( 0x40000, REGION_CPU3, 0 ) /* 2nd 68000 code */
	ROM_LOAD16_BYTE( "11109.20", 0x00000, 0x20000, CRC(85a0fe07) SHA1(5a3a8fda6cb4898cfece4ec865b81b9b60f9ad55) )
	ROM_LOAD16_BYTE( "11110.29", 0x00001, 0x20000, CRC(f3d6797c) SHA1(17487b89ddbfbcc32a0b52268259f1c8d10fd0b2) )

	ROM_REGION( 0x10000, REGION_GFX3, 0 ) /* road gfx */
	ROM_LOAD( "10922.40", 0x000000, 0x10000, CRC(b49183d4) SHA1(71d87bfbce858049ccde9597ab15575b3cdba892) )
ROM_END

/*****************************************************************************/
/* Line of Fire */
ROM_START( loffire )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr12850.rom", 0x000000, 0x20000, CRC(14598f2a) )
	ROM_LOAD16_BYTE( "epr12849.rom", 0x000001, 0x20000, CRC(61cfd2fe) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "opr12791.rom", 0x00000, 0x10000, CRC(acfa69ba) )
	ROM_LOAD( "opr12792.rom", 0x10000, 0x10000, CRC(e506723c) )
	ROM_LOAD( "opr12793.rom", 0x20000, 0x10000, CRC(0ce8cce3) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "epr12775.rom", 0x000001, 0x20000, CRC(693056ec) )
	ROM_LOAD16_BYTE( "epr12776.rom", 0x000000, 0x20000, CRC(61efbdfd) )
	ROM_LOAD16_BYTE( "epr12777.rom", 0x040001, 0x20000, CRC(29d5b953) )
	ROM_LOAD16_BYTE( "epr12778.rom", 0x040000, 0x20000, CRC(2fb68e07) )
	ROM_LOAD16_BYTE( "epr12779.rom", 0x080001, 0x20000, CRC(ae58af7c) )
	ROM_LOAD16_BYTE( "epr12780.rom", 0x080000, 0x20000, CRC(ee670c1e) )
	ROM_LOAD16_BYTE( "epr12781.rom", 0x0c0001, 0x20000, CRC(538f6bc5) )
	ROM_LOAD16_BYTE( "epr12782.rom", 0x0c0000, 0x20000, CRC(5acc34f7) )
	ROM_LOAD16_BYTE( "epr12783.rom", 0x100001, 0x20000, CRC(c13feea9) )
	ROM_LOAD16_BYTE( "epr12784.rom", 0x100000, 0x20000, CRC(39b94c65) )
	ROM_LOAD16_BYTE( "epr12785.rom", 0x140001, 0x20000, CRC(05ed0059) )
	ROM_LOAD16_BYTE( "epr12786.rom", 0x140000, 0x20000, CRC(a4123165) )
	ROM_LOAD16_BYTE( "epr12787.rom", 0x180001, 0x20000, CRC(6431a3a6) )
	ROM_LOAD16_BYTE( "epr12788.rom", 0x180000, 0x20000, CRC(1982a0ce) )
	ROM_LOAD16_BYTE( "epr12789.rom", 0x1c0001, 0x20000, CRC(97d03274) )
	ROM_LOAD16_BYTE( "epr12790.rom", 0x1c0000, 0x20000, CRC(816e76e6) )

	ROM_REGION( 0x70000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr12798.rom",	 0x00000, 0x10000, CRC(0587738d) )
	ROM_LOAD( "epr12799.rom",    0x10000, 0x20000, CRC(bc60181c) )
	ROM_LOAD( "epr12800.rom",    0x30000, 0x20000, CRC(1158c1a3) )
	ROM_LOAD( "epr12801.rom",    0x50000, 0x20000, CRC(2d6567c4) )

	ROM_REGION( 0x100000, REGION_CPU3, 0 ) /* 2nd 68000 code */
	ROM_LOAD16_BYTE( "epr12803.rom", 0x000000, 0x20000, CRC(c1d9e751) )
	ROM_LOAD16_BYTE( "epr12802.rom", 0x000001, 0x20000, CRC(d746bb39) )
	ROM_LOAD16_BYTE( "epr12805.rom", 0x040000, 0x20000, CRC(4a7200c3) )
	ROM_LOAD16_BYTE( "epr12804.rom", 0x040001, 0x20000, CRC(b853480e) )
ROM_END

/*****************************************************************************/
// Thunder Blade
ROM_START( thndrbld )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "thnbld.58", 0x000000, 0x20000, CRC(e057dd5a) )
	ROM_LOAD16_BYTE( "thnbld.63", 0x000001, 0x20000, CRC(c6b994b8) )
	ROM_LOAD16_BYTE( "11306.epr", 0x040000, 0x20000, CRC(4b95f2b4) )
	ROM_LOAD16_BYTE( "11307.epr", 0x040001, 0x20000, CRC(2d6833e4) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "11316.epr", 0x00000, 0x10000, CRC(84290dff) )
	ROM_LOAD( "11315.epr", 0x10000, 0x10000, CRC(35813088) )
	ROM_LOAD( "11314.epr", 0x20000, 0x10000, CRC(d4f954a9) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "thnbld.105",0x000001, 0x20000, CRC(b4a382f7) )
	ROM_LOAD16_BYTE( "thnbld.101",0x000000, 0x20000, CRC(525e2e1d) )
	ROM_LOAD16_BYTE( "thnbld.97", 0x040001, 0x20000, CRC(5f2783be) )
	ROM_LOAD16_BYTE( "thnbld.93", 0x040000, 0x20000, CRC(90775579) )
	ROM_LOAD16_BYTE( "11328.epr", 0x080001, 0x20000, CRC(da39e89c) )
	ROM_LOAD16_BYTE( "11329.epr", 0x080000, 0x20000, CRC(31b20257) )
	ROM_LOAD16_BYTE( "11330.epr", 0x0c0001, 0x20000, CRC(aa7c70c5) )
	ROM_LOAD16_BYTE( "11331.epr", 0x0c0000, 0x20000, CRC(3a2c042e) )
	ROM_LOAD16_BYTE( "11324.epr", 0x100001, 0x20000, CRC(9742b552) )
	ROM_LOAD16_BYTE( "11325.epr", 0x100000, 0x20000, CRC(b9e98ae9) )
	ROM_LOAD16_BYTE( "11326.epr", 0x140001, 0x20000, CRC(29198403) )
	ROM_LOAD16_BYTE( "11327.epr", 0x140000, 0x20000, CRC(deae90f1) )
	ROM_LOAD16_BYTE( "11320.epr", 0x180001, 0x20000, CRC(a95c76b8) )
//	ROM_LOAD16_BYTE( "11321.epr", 0x180000, 0x20000, CRC(8e738f58) )
	ROM_LOAD16_BYTE( "thnbld.98", 0x180000, 0x10000, CRC(eb4b9e57) )
	ROM_LOAD16_BYTE( "11322.epr", 0x1c0001, 0x20000, CRC(10364d74) )
	ROM_LOAD16_BYTE( "11323.epr", 0x1c0000, 0x20000, CRC(27e40735) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "thnbld.17",	 0x00000, 0x10000, CRC(d37b54a4) )

	ROM_REGION( 0x60000, REGION_SOUND1, 0 ) /* Sega PCM sound data */
	ROM_LOAD( "11317.epr",   0x00000, 0x20000, CRC(d4e7ac1f) )
	ROM_LOAD( "11318.epr",   0x20000, 0x20000, CRC(70d3f02c) )
	ROM_LOAD( "11319.epr",   0x40000, 0x20000, CRC(50d9242e) )

	ROM_REGION( 0x100000, REGION_CPU3, 0 ) /* 2nd 68000 code */
	ROM_LOAD16_BYTE( "thnbld.20", 0x000000, 0x20000, CRC(ed988fdb) )
	ROM_LOAD16_BYTE( "thnbld.29", 0x000001, 0x20000, CRC(12523bc1) )
	ROM_LOAD16_BYTE( "11310.epr", 0x040000, 0x20000, CRC(5d9fa02c) )
	ROM_LOAD16_BYTE( "11311.epr", 0x040001, 0x20000, CRC(483de21b) )

	ROM_REGION( 0x10000, REGION_GFX3, 0 ) /* ???? */
	ROM_LOAD( "11313.epr",	 0x00000, 0x10000, CRC(6a56c4c3) )
ROM_END

// Thunder Blade Japan
ROM_START( thndrbdj )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "11304.epr", 0x00000, 0x20000, CRC(a90630ef) ) // patched
	ROM_LOAD16_BYTE( "11306.epr", 0x40000, 0x20000, CRC(4b95f2b4) ) // patched
	ROM_LOAD16_BYTE( "11305.epr", 0x00001, 0x20000, CRC(9ba3ef61) )
	ROM_LOAD16_BYTE( "11307.epr", 0x40001, 0x20000, CRC(2d6833e4) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "11314.epr", 0x00000, 0x10000, CRC(d4f954a9) )
	ROM_LOAD( "11315.epr", 0x10000, 0x10000, CRC(35813088) )
	ROM_LOAD( "11316.epr", 0x20000, 0x10000, CRC(84290dff) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD( "11323.epr", 0x20000*0x0, 0x20000, CRC(27e40735) )
	ROM_LOAD( "11322.epr", 0x20000*0x4, 0x20000, CRC(10364d74) )
	ROM_LOAD( "11321.epr", 0x20000*0x8, 0x20000, CRC(8e738f58) )
	ROM_LOAD( "11320.epr", 0x20000*0xc, 0x20000, CRC(a95c76b8) )
	ROM_LOAD( "11327.epr", 0x20000*0x1, 0x20000, CRC(deae90f1) )
	ROM_LOAD( "11326.epr", 0x20000*0x5, 0x20000, CRC(29198403) )
	ROM_LOAD( "11325.epr", 0x20000*0x9, 0x20000, CRC(b9e98ae9) )
	ROM_LOAD( "11324.epr", 0x20000*0xd, 0x20000, CRC(9742b552) )
	ROM_LOAD( "11331.epr", 0x20000*0x2, 0x20000, CRC(3a2c042e) )
	ROM_LOAD( "11330.epr", 0x20000*0x6, 0x20000, CRC(aa7c70c5) )
	ROM_LOAD( "11329.epr", 0x20000*0xa, 0x20000, CRC(31b20257) )
	ROM_LOAD( "11328.epr", 0x20000*0xe, 0x20000, CRC(da39e89c) )
	ROM_LOAD( "11335.epr", 0x20000*0x3, 0x20000, CRC(f19b3e86) )
	ROM_LOAD( "11334.epr", 0x20000*0x7, 0x20000, CRC(348f91c7) )
	ROM_LOAD( "11333.epr", 0x20000*0xb, 0x20000, CRC(05a2333f) )
	ROM_LOAD( "11332.epr", 0x20000*0xf, 0x20000, CRC(dc089ec6) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "11312.epr",   0x00000, 0x10000, CRC(3b974ed2) )

	ROM_REGION( 0x60000, REGION_SOUND1, 0 ) /* Sega PCM sound data */
	ROM_LOAD( "11317.epr",   0x00000, 0x20000, CRC(d4e7ac1f) )
	ROM_LOAD( "11318.epr",   0x20000, 0x20000, CRC(70d3f02c) )
	ROM_LOAD( "11319.epr",   0x40000, 0x20000, CRC(50d9242e) )

	ROM_REGION( 0x80000, REGION_CPU3, 0 ) /* 2nd 68000 code */
	ROM_LOAD16_BYTE( "11308.epr", 0x00000, 0x20000, CRC(7956c238) )
	ROM_LOAD16_BYTE( "11310.epr", 0x40000, 0x20000, CRC(5d9fa02c) )
	ROM_LOAD16_BYTE( "11309.epr", 0x00001, 0x20000, CRC(c887f620) )
	ROM_LOAD16_BYTE( "11311.epr", 0x40001, 0x20000, CRC(483de21b) )

	ROM_REGION( 0x10000, REGION_GFX3, 0 ) /* ground data */
	ROM_LOAD( "11313.epr",	 0x00000, 0x10000, CRC(6a56c4c3) )
ROM_END

/*****************************************************************************/

#if 0
static READ16_HANDLER( aburner_shareram_r ){
	return sys16_extraram[offset];
}
static WRITE16_HANDLER( aburner_shareram_w ){
	COMBINE_DATA( &sys16_extraram[offset] );
}

static READ16_HANDLER( aburner_road_r ){
	return sys16_extraram2[offset];
}
static WRITE16_HANDLER( aburner_road_w ){
	COMBINE_DATA( &sys16_extraram2[offset] );
}
#endif

INPUT_PORTS_START( aburner )
	PORT_START /* player 1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 ) /* unknown */
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 ) /* service */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) /* vulcan */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) /* missle */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	SYS16_COINAGE /* DSWA */ /* wrong! */

	PORT_START /* DSWB */
	PORT_DIPNAME( 0x03, 0x01, "Cabinet Type" )
	PORT_DIPSETTING(    0x01, "Upright 1" )
	PORT_DIPSETTING(    0x00, "N/A" )
	PORT_DIPSETTING(    0x02, "Moving Standard" )
	PORT_DIPSETTING(    0x03, "Moving Deluxe" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "Ship Increase" )
	PORT_DIPSETTING(    0x10, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x20, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x80, "Easy" )
	PORT_DIPSETTING(    0xc0, "Normal" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )

	PORT_START
	PORT_ANALOG( 0xff, 0x7f, IPT_AD_STICK_X | IPF_CENTER, 100, 4, 0x00, 0xff )

	PORT_START
	PORT_ANALOG( 0xff, 0x7f, IPT_AD_STICK_Y | IPF_CENTER | IPF_REVERSE, 100, 4, 0x00, 0xff )

	PORT_START /* throttle (hack - mapped as player2 stick) */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_Y | IPF_CENTER | IPF_PLAYER2, 100, 79, 0x00, 0xff )
INPUT_PORTS_END

INPUT_PORTS_START( aburner2 )
	PORT_START /* player 1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 ) /* unknown */
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 ) /* service */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) /* vulcan */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) /* missle */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START /* DSWA */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x05, "6 Coins 4 Credits" ) //DEF_STR( 6C_4C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 4C_5C ) )
	PORT_DIPSETTING(    0x03, "5 Coins 6 Credits" ) //DEF_STR( 5C_6C ) )
	//PORT_DIPSETTING(    0x01, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x00, "Free Play (if Coin B too) or 1/1" )

	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x50, "6 Coins 4 Credits" ) //DEF_STR( 6C_4C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 4C_3C ) ) // 1.33
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) ) // 1
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_5C ) ) // .8
	PORT_DIPSETTING(    0x30, "5 Coins 6 Credits" ) // 8.3 DEF_STR( 5C_6C ) )
	//PORT_DIPSETTING(    0x10, DEF_STR( 2C_3C ) ) // .66
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_3C ) ) // .66
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) ) // .5
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) ) // .33
	PORT_DIPSETTING(    0x00, "Free Play (if Coin A too) or 1/1" )
	/* note that Free Play doesn't seem to work! */

	PORT_START /* DSWB */
	PORT_DIPNAME( 0x03, 0x01, "Cabinet Type" )
	PORT_DIPSETTING(    0x01, "Upright 1" )
	PORT_DIPSETTING(    0x00, "Upright 2" )
	PORT_DIPSETTING(    0x02, "Moving Standard" )
	PORT_DIPSETTING(    0x03, "Moving Deluxe" )
	PORT_DIPNAME( 0x04, 0x04, "Throttle Lever" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x10, 0x00, "Ship Increase" )
	PORT_DIPSETTING(    0x10, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x20, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x80, "Easy" )
	PORT_DIPSETTING(    0xc0, "Normal" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )

	PORT_START
	PORT_ANALOG( 0xff, 0x7f, IPT_AD_STICK_X | IPF_CENTER, 100, 4, 0x00, 0xff )

	PORT_START
	PORT_ANALOG( 0xff, 0x7f, IPT_AD_STICK_Y | IPF_CENTER | IPF_REVERSE, 100, 4, 0x00, 0xff )

	PORT_START /* throttle (hack - mapped as player2 stick) */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_Y | IPF_CENTER | IPF_PLAYER2, 100, 79, 0x00, 0xff )
INPUT_PORTS_END

INPUT_PORTS_START( thndrbld )
	PORT_START /* player 1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 ) /* unknown */
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 ) /* service */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) /* gun */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) /* missle */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	SYS16_COINAGE /* DSWA */ /* wrong! */

	PORT_START /* DSWB */
	PORT_DIPNAME( 0x01, 0x01, "Cabinet Type" )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, "Deluxe" )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Time" )
	PORT_DIPSETTING(    0x00, "0 Seconds" )
	PORT_DIPSETTING(    0x04, "30 Seconds" )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x18, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x20, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Yes ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x40, "Easy" )
	PORT_DIPSETTING(    0xc0, "Normal" )
	PORT_DIPSETTING(    0x80, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )

	PORT_START
	PORT_ANALOG( 0xff, 0x7f, IPT_AD_STICK_X | IPF_CENTER | IPF_REVERSE, 100, 4, 0x00, 0xff )

	PORT_START /* throttle (hack - mapped as player2 stick) */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_Y | IPF_CENTER | IPF_PLAYER2, 100, 79, 0x00, 0xff )

	PORT_START
	PORT_ANALOG( 0xff, 0x7f, IPT_AD_STICK_Y | IPF_CENTER, 100, 4, 0x00, 0xff )
INPUT_PORTS_END

/*****************************************************************************/
/* aburner hardware motor/moving cockpit abstraction */

static WRITE16_HANDLER( aburner_motor_power_w ){
	/*	most significant 4 bits:
			vertical motor speed
				0x4 (up, fast)
				0x5
				0x6
				0x7 (up, slow)
				0x8 (idle)
				0x9 (down, slow)
				0xa
				0xb
				0xc (down, fast)

		least significant 4 bits:
			horizontal motor speed
				0x4 (right, fast)
				0x5
				0x6
				0x7 (right, slow)
				0x8 (idle)
				0x9 (left, slow)
				0xa
				0xb
				0xc (left, fast)
	*/
}

static READ16_HANDLER( aburner_motor_status_r ){
	/* --L-RD-U
		each bit, if set, indicates that the moving cockpit has reached it's
		extreme position in a particular direction
		(i.e. the motor can't move it further)
	*/
	return 0x2d;
}
static UINT8 aburner_motor_xpos( void ){ /* poll cockpit horizontal position */
	return (0xb0+0x50)/2; /* expected values are in the range 0x50..0xb0 */
}
static UINT8 aburner_motor_ypos( void ){ /* poll cockpit vertical position */
	return (0xb0+0x50)/2; /* expected values are in the range 0x50..0xb0 */
}

/*****************************************************************************/

static int sys16_analog_select;
static WRITE16_HANDLER( aburner_analog_select_w ){
	if( ACCESSING_LSB ) sys16_analog_select = data&0xff;
}

static READ16_HANDLER( aburner_analog_r ){
	switch( sys16_analog_select ){
	case 0x00: return readinputport(3);
	case 0x04: return readinputport(4);
	case 0x08: return readinputport(5);
	case 0x0c: return aburner_motor_ypos();
	case 0x10: return aburner_motor_xpos();
	default: return 0x00; /* unused? */
	}
}

/*****************************************************************************/

UINT16 aburner_unknown;
UINT16 aburner_lamp;

static WRITE16_HANDLER( aburner_unknown_w ){
	COMBINE_DATA( &aburner_unknown );
}
static WRITE16_HANDLER( aburner_lamp_w ){
	COMBINE_DATA( &aburner_lamp );
}

/*****************************************************************************/

/* math coprocessor for afterburner hardware */

static struct math_context {
	UINT16 product[4];
	UINT16 quotient[4];	/* operand0_hi, operand0_lo, operand1 */
	UINT16 compare[4];	/* F,G,H */
} math0_context, math1_context;

static WRITE16_HANDLER( math0_product_w ){
	COMBINE_DATA( &math0_context.product[offset&3]);
}
static WRITE16_HANDLER( math0_quotient_w ){
	COMBINE_DATA( &math0_context.quotient[offset&3]);
}
static WRITE16_HANDLER( math0_compare_w ){
	offset&=3;
	COMBINE_DATA( &math0_context.compare[offset]);
	if( offset==3 ){
		soundlatch_w( 0,math0_context.compare[3]&0xff );
		cpu_set_nmi_line(1, PULSE_LINE);
	}
}
static READ16_HANDLER( math0_product_r ){
	UINT32 result = ((INT16)math0_context.product[0])*((INT16)math0_context.product[1]);
	switch( offset&3 ){
	case 0: return math0_context.product[0];
	case 1: return math0_context.product[1];
	case 2: return result>>16;
	case 3: return result&0xffff;
	}
	return 0;
}
static READ16_HANDLER( math0_quotient_r ){
	INT32 operand0 = (math0_context.quotient[0]<<16)|math0_context.quotient[1];
	switch( offset&7 ){
	case 0: case 1: case 2: case 3:
		return math0_context.quotient[offset];
	case 4: return math0_context.quotient[2] ? (UINT16)(operand0/(INT16)math0_context.quotient[2]) : 0x7fff;
	case 5: return math0_context.quotient[2] ? (UINT16)(operand0%(INT16)math0_context.quotient[2]) : 0x0000;
	}
	logerror( "unknown quotient_r\n" );
	return 0;
}
static READ16_HANDLER( math0_compare_r ){ /* 0xe8006 */
	switch( offset&3 ){
	case 0: return math0_context.compare[0];
	case 1: return math0_context.compare[1];
	case 2: return math0_context.compare[2];
	case 3:
		{
			INT16 F = math0_context.compare[0]; /* range min */
			INT16 G = math0_context.compare[1]; /* range max */
			INT16 H = math0_context.compare[2]; /* test value */
			if( F<=G ){
				if( H<F ) return (UINT16)-1;
				if( H>G ) return 1;
			}
			else {
				if( H<0 ) return (UINT16)-1;
				if( H>0 ) return 1;
			}
		}
		break;
	}

	return 0;
}

/* 2nd chip */
static WRITE16_HANDLER( math1_product_w ){
	COMBINE_DATA( &math1_context.product[offset&3]);
}
static WRITE16_HANDLER( math1_quotient_w ){
//	if( offset == 6 ) offset = 2; // tblade
	COMBINE_DATA( &math1_context.quotient[offset&3]);
}
static WRITE16_HANDLER( math1_compare_w ){
	offset&=3;
	COMBINE_DATA( &math1_context.compare[offset]);
	if( offset==3 ){
		soundlatch_w( 0,math1_context.compare[3]&0xff );
		cpu_set_nmi_line(1, PULSE_LINE);
	}
}
static READ16_HANDLER( math1_product_r ){
	UINT32 result = ((INT16)math1_context.product[0])*((INT16)math1_context.product[1]);
	switch( offset&3 ){
	case 0: return math1_context.product[0];
	case 1: return math1_context.product[1];
	case 2: return result>>16;
	case 3: return result&0xffff;
	}
	return 0;
}
static READ16_HANDLER( math1_quotient_r ){
	INT32 operand0 = (math1_context.quotient[0]<<16)|math1_context.quotient[1];
	switch( offset&7 ){
	case 0: case 1: case 2: case 3:
		return math1_context.quotient[offset];
	case 4: return math1_context.quotient[2] ? (UINT16)(operand0/(INT16)math1_context.quotient[2]) : 0x7fff;
	case 5: return math1_context.quotient[2] ? (UINT16)(operand0%(INT16)math1_context.quotient[2]) : 0x0000;
	}
	logerror( "unknown quotient_r\n" );
	return 0;
}
static READ16_HANDLER( math1_compare_r ){ /* 0xe8006 */
	switch( offset&3 ){
	case 0: return math1_context.compare[0];
	case 1: return math1_context.compare[1];
	case 2: return math1_context.compare[2];
	case 3:
		{
			INT16 F = math1_context.compare[0]; /* range min */
			INT16 G = math1_context.compare[1]; /* range max */
			INT16 H = math1_context.compare[2]; /* test value */
			if( F<=G ){
				if( H<F ) return (UINT16)-1;
				if( H>G ) return 1;
			}
			else {
				if( H<0 ) return (UINT16)-1;
				if( H>0 ) return 1;
			}
		}
		break;
	}

	return 0;
}

static MEMORY_READ16_START( aburner_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x0c0000, 0x0cffff, SYS16_MRA16_TILERAM },			/* 16 tilemaps */
	{ 0x0d0000, 0x0d0fff, SYS16_MRA16_TEXTRAM },

	{ 0x0e0000, 0x0e001f, math0_product_r },
	{ 0x0e4000, 0x0e401f, math0_quotient_r },
	{ 0x0e8000, 0x0e801f, math0_compare_r },

	{ 0x100000, 0x101fff, SYS16_MRA16_SPRITERAM },
	{ 0x120000, 0x12401f, SYS16_MRA16_PALETTERAM },
	{ 0x130000, 0x130001, aburner_analog_r },
	{ 0x140000, 0x140001, aburner_motor_status_r },
	{ 0x150000, 0x150001, input_port_0_word_r },			/* buttons */
	{ 0x150004, 0x150005, input_port_1_word_r },			/* DSW A */
	{ 0x150006, 0x150007, input_port_2_word_r },			/* DSW B */
	{ 0x200000, 0x27ffff, SYS16_CPU3ROM16_r },				/* CPU2 ROM */
	{ 0x29c000, 0x2a3fff, SYS16_MRA16_WORKINGRAM2 },

	{ 0x2e0000, 0x2e001f, math1_product_r },
	{ 0x2e4000, 0x2e401f, math1_quotient_r },
	{ 0x2e8000, 0x2e801f, math1_compare_r },

	{ 0x2ec000, 0x2ee001, SYS16_MRA16_ROADRAM },
	{ 0xe00000, 0xe00001, SYS16_CPU2_RESET_HACK },			/* hack! */
	{ 0xff8000, 0xffffff, SYS16_MRA16_WORKINGRAM },
MEMORY_END

/*
	58,63 are CPU1 patched ROMs

	RAM:
		22,23
		31,32
		38,39	spriteram
		55,60	workingram
		56,61
		125,126	roadram
		132,133	textram
		134,135	tileram
		150,151	palette

	custom:
		37:		?
		41:		?
		53:		math chip0 compare?
		107:	math chip0 product
		108:	math chip0 divide?
*/
static MEMORY_WRITE16_START( aburner_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x0c0000, 0x0cffff, SYS16_MWA16_TILERAM },	/* 134,135 */
	{ 0x0d0000, 0x0d0fff, SYS16_MWA16_TEXTRAM },	/* 132,133 */

	{ 0x0e0000, 0x0e001f, math0_product_w },
	{ 0x0e4000, 0x0e401f, math0_quotient_w },
	{ 0x0e8000, 0x0e801f, math0_compare_w },		/* includes sound latch! (0x0e8016) */

	{ 0x100000, 0x101fff, SYS16_MWA16_SPRITERAM },		/* 38,39 */
	{ 0x110000, 0x110001, MWA16_NOP },				/* unknown */
	{ 0x120000, 0x12401f, SYS16_MWA16_PALETTERAM },	/* 150, 151 */
	{ 0x130000, 0x130001, aburner_analog_select_w },
	{ 0x140002, 0x140003, aburner_motor_power_w },
	{ 0x140004, 0x140005, aburner_unknown_w },		/* unknown */
	{ 0x140006, 0x140007, aburner_lamp_w },			/* 0x06 - start lamp, warning lamp */
	{ 0x200000, 0x27ffff, MWA16_ROM },				/* CPU2 ROM */
	{ 0x29c000, 0x2a3fff, SYS16_MWA16_WORKINGRAM2 },

	{ 0x2e0000, 0x2e001f, math1_product_w },
	{ 0x2e4000, 0x2e401f, math1_quotient_w },
	{ 0x2e8000, 0x2e801f, math1_compare_w },		/* includes sound latch! */

	{ 0x2ec000, 0x2ee001, SYS16_MWA16_ROADRAM },	/* 125,126 */
	{ 0xff8000, 0xffffff, SYS16_MWA16_WORKINGRAM },	/* 55,60 */
MEMORY_END

static MEMORY_READ16_START( aburner_readmem2 )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x09c000, 0x0a3fff, SYS16_MRA16_WORKINGRAM2_SHARE },

	{ 0x0e0000, 0x0e001f, math1_product_r },
	{ 0x0e4000, 0x0e401f, math1_quotient_r },
	{ 0x0e8000, 0x0e801f, math1_compare_r },

	{ 0x0ec000, 0x0ee001, SYS16_MRA16_ROADRAM_SHARE },
	{ 0x200000, 0x27ffff, MRA16_ROM }, /* mirror */
	{ 0x29c000, 0x2a3fff, SYS16_MRA16_WORKINGRAM2_SHARE }, /* mirror */
MEMORY_END

static MEMORY_WRITE16_START( aburner_writemem2 )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x09c000, 0x0a3fff, SYS16_MWA16_WORKINGRAM2_SHARE },

	{ 0x0e0000, 0x0e001f, math1_product_w },
	{ 0x0e4000, 0x0e401f, math1_quotient_w },
	{ 0x0e8000, 0x0e801f, math1_compare_w },

	{ 0x0ec000, 0x0ee001, SYS16_MWA16_ROADRAM_SHARE },
	{ 0x29c000, 0x2a3fff, SYS16_MWA16_WORKINGRAM2_SHARE }, /* mirror */
MEMORY_END

static MEMORY_READ_START( aburner_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xf000, 0xf0ff, SegaPCM_r },
	{ 0xf000, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( aburner_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xf000, 0xf0ff, SegaPCM_w },
	{ 0xf000, 0xffff, MWA_RAM },
MEMORY_END

static PORT_READ_START( aburner_sound_readport )
	{ 0x01, 0x01, YM2151_status_port_0_r },
	{ 0x40, 0x40, soundlatch_r },
PORT_END

static PORT_WRITE_START( aburner_sound_writeport )
	{ 0x00, 0x00, YM2151_register_port_0_w },
	{ 0x01, 0x01, YM2151_data_port_0_w },
PORT_END

/***************************************************************************/

static MACHINE_INIT( aburner ){
	sys16_textmode = 2;
	sys16_spritesystem = sys16_sprite_aburner;
	sys16_sprxoffset = -0xc0;

	sys16_fgxoffset = 8;
	sys16_textlayer_lo_min=0;
	sys16_textlayer_lo_max=0;
	sys16_textlayer_hi_min=0;
	sys16_textlayer_hi_max=0xff;
}

static DRIVER_INIT( thndrbdj ){
	machine_init_sys16_onetime();
	sys16_bg1_trans = 1;
	sys16_interleave_sprite_data( 0x200000 );
}

static DRIVER_INIT( aburner ){
	/* reset hack for AfterBurner */
	sys16_patch_code(0xe76c,0x4a);
	sys16_patch_code(0xe76d,0x79);
	sys16_patch_code(0xe76e,0x00);
	sys16_patch_code(0xe76f,0xe0);
	sys16_patch_code(0xe770,0x00);
	sys16_patch_code(0xe771,0x00);

	machine_init_sys16_onetime();
	sys16_bg1_trans = 1;
	sys16_interleave_sprite_data( 0x200000 );
}

static DRIVER_INIT( aburner2 ){
	/* reset hack for AfterBurner2 */
	sys16_patch_code(0x1483c,0x4a);
	sys16_patch_code(0x1483d,0x79);
	sys16_patch_code(0x1483e,0x00);
	sys16_patch_code(0x1483f,0xe0);
	sys16_patch_code(0x14840,0x00);
	sys16_patch_code(0x14841,0x00);

	machine_init_sys16_onetime();
	sys16_bg1_trans = 1;
	sys16_interleave_sprite_data( 0x200000 );
}

INTERRUPT_GEN( aburner_interrupt ){
	if( cpu_getiloops()!=0 )
		irq2_line_hold(); /* (?) updates sound and inputs */
	else
		irq4_line_hold(); /* vblank */
}

static MACHINE_DRIVER_START( aburner )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_MEMORY(aburner_readmem,aburner_writemem)
	MDRV_CPU_VBLANK_INT(aburner_interrupt,7)
	
	MDRV_CPU_ADD(Z80, 4096000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(aburner_sound_readmem, aburner_sound_writemem)
	MDRV_CPU_PORTS(aburner_sound_readport, aburner_sound_writeport)
	
	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_MEMORY(aburner_readmem2,aburner_writemem2)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)
	
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	MDRV_MACHINE_INIT(aburner)
	
	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(sys16_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x2010)
	
	MDRV_VIDEO_START(aburner)
	MDRV_VIDEO_UPDATE(aburner)
	
	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, sys16_ym2151_interface)
	MDRV_SOUND_ADD(SEGAPCM, sys16_segapcm_interface_15k_512)
MACHINE_DRIVER_END

/*          rom       parent    machine   inp       init */
GAME( 1987, aburner,  aburner2, aburner,  aburner,  aburner,  ROT0, "Sega", "After Burner (Japan)" )
GAME( 1987, aburner2, 0,        aburner,  aburner2, aburner2, ROT0, "Sega", "After Burner II" )
GAMEX(19??, loffire,  0,        aburner,  aburner,  aburner,  ROT0, "Sega", "Line of Fire", GAME_NOT_WORKING )
GAMEX(19??, thndrbld, 0,        aburner,  thndrbld, aburner,  ROT0, "Sega", "Thunder Blade", GAME_NOT_WORKING )
GAME( 19??, thndrbdj, thndrbld, aburner,  thndrbld, thndrbdj, ROT0, "Sega", "Thunder Blade (Japan)" )
