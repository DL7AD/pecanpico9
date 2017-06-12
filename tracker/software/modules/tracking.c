#include "ch.h"
#include "hal.h"

#include "debug.h"
#include "ptime.h"
#include "config.h"
#include "max.h"
#include "bme280.h"
#include "padc.h"
#include "pac1720.h"
#include "radio.h"
#include "flash.h"
#include "watchdog.h"

static trackPoint_t trackPoints[2];
static trackPoint_t* lastTrackPoint;
static systime_t nextLogEntryTimer;
static module_conf_t trac_conf = {.name = "TRAC"}; // Fake config needed for watchdog tracking

/*
 * Sequence determines in which order log packets are sent out
 * This sequence sorts all log messages in a way, that gaps in the log are
 * equally distributed over the route, if not all packets could be received
 * by the APRS network.
 */
static const uint16_t log_transmission_sequence[] = {
	731,2148,407,645,1599,2319,2970,324,1489,2143,60,1269,463,1126,2302,2115,77,837,445,2180,
	2451,85,2465,413,2361,526,702,1768,475,916,507,1675,2429,1350,546,1516,1431,1793,2831,2958,
	2176,2762,1009,1936,415,334,1780,1322,2754,965,2606,1790,2035,2203,276,750,501,1156,63,1424,
	438,1399,1854,1760,644,329,3023,2067,2463,2843,517,1453,490,319,2037,1542,2296,246,810,2895,
	361,3070,520,2858,2237,498,2178,381,1400,1772,2834,1527,1634,1600,851,1993,1061,976,1859,2499,
	2459,2484,2100,336,1853,2634,2232,911,1240,2349,1121,370,1151,2482,944,1406,826,1656,741,2772,
	2388,2748,591,3019,2228,1375,92,2290,1086,2411,2813,999,574,686,2698,649,84,1612,2512,283,
	690,354,518,1084,2885,2601,1693,132,33,2507,252,776,1705,1270,884,2756,938,988,748,1922,
	1124,2215,2789,400,108,845,200,2666,444,2189,1587,1397,1680,216,1619,716,1012,2659,857,465,
	674,2887,551,1710,945,61,3065,2740,2556,377,2183,2235,3010,1687,1716,339,263,96,2799,694,
	738,2502,1183,2316,1873,683,767,1178,2540,2526,2401,2510,2426,2504,1927,1485,1435,1255,2239,2663,
	1051,2284,245,2046,2117,2830,2880,1529,1801,1437,1961,1534,2509,1318,3038,700,129,1013,2345,2410,
	2757,2527,2960,152,2704,1017,1559,568,1449,267,447,1452,2057,371,2676,1647,578,1947,1785,1229,
	2241,1907,455,2167,1691,842,1390,2696,2355,2992,1188,1053,3027,2181,858,2260,2359,2407,2518,2322,
	561,2471,1888,1837,2776,2803,115,1519,1536,1501,17,576,1582,3063,2077,1484,2987,1414,1803,2982,
	2651,457,385,1547,2457,1428,1725,708,2820,948,1879,2983,2661,1157,360,2139,1378,1383,2897,433,
	2959,1697,42,2917,1092,1825,2879,1407,337,917,1953,1110,1777,2771,2528,745,566,643,2536,194,
	1949,2741,294,28,2705,1058,2269,2562,1341,343,502,1396,557,2192,2450,768,2894,2814,2055,3011,
	2876,23,2332,2662,1880,405,356,40,1792,1664,1943,2769,3025,723,2633,1189,3021,2580,239,3050,
	1429,303,459,2372,1050,489,2247,2134,2732,597,882,2702,1190,2901,2155,575,1332,443,506,1133,
	1526,2193,2470,2888,1848,737,1440,1921,2103,2015,1865,167,618,1098,1670,724,1569,1627,172,2280,
	1741,2128,1135,416,1568,2922,2628,961,2185,1918,855,1808,2272,1906,982,2986,1973,523,1881,713,
	2135,2087,2918,2483,1505,701,1804,1083,349,2142,124,227,2911,846,675,808,1033,802,705,2313,
	2182,2381,1731,2729,2083,967,321,1778,1685,1727,187,1500,1225,1461,1184,2036,1211,1115,3000,384,
	1327,2695,1317,1148,1724,1016,806,732,374,2826,2520,1127,1475,1287,522,865,2767,1863,901,1007,
	2715,604,2523,1980,2890,3069,2782,613,1072,1118,2299,689,269,2995,1125,1340,687,1019,1591,796,
	2554,380,224,1912,2306,101,435,2305,135,1325,484,1469,1089,2,820,619,2413,2298,784,2954,
	874,226,1026,2390,2364,511,8,71,2427,1412,1232,1480,977,1112,1079,586,155,481,2329,1374,
	2195,2564,1610,139,1252,1744,3044,301,1847,843,1688,679,1776,1373,2976,2701,2607,2925,2013,1586,
	1316,1570,1871,2493,1583,2108,629,3005,2068,730,1631,1305,271,2442,2945,1153,2304,2805,867,2066,
	1020,712,2861,2017,1324,2165,1528,1809,2875,2714,2969,3029,1117,630,2397,1018,1915,2336,382,1962,
	2753,261,3066,2516,1314,2218,1517,186,1005,1024,410,1042,83,1931,2075,207,1541,437,2334,2599,
	524,1737,3034,2088,1457,2255,2282,606,2440,889,2727,1740,307,393,1187,1508,850,253,2941,46,
	614,2802,1165,2478,514,1254,39,1463,508,1827,369,2277,2550,2989,2801,673,2567,169,2436,2557,
	1004,2202,2357,676,1828,3008,2794,1593,146,2008,292,1545,2877,2882,1377,4,2175,494,1023,2264,
	2962,1733,2351,1478,427,1411,2362,1035,2110,838,1575,2194,2821,317,1310,1703,2275,812,41,257,
	2836,903,1223,2273,1260,1100,383,3013,195,2323,2817,1271,1678,1757,1289,2259,1944,2211,1551,1313,
	1523,1152,1581,1584,2097,1158,1366,2380,1892,1006,2972,627,794,2977,1617,2219,535,743,1566,1028,
	2474,1864,612,206,1298,684,3028,2604,2916,3055,787,1783,1299,500,617,2352,759,885,2923,268,
	1543,2085,2681,1160,622,2884,282,1712,1108,1066,2752,2023,2832,158,1858,1762,2866,2806,2618,2996,
	2119,2496,2586,1222,589,3058,2736,1730,482,2070,36,2220,254,2171,1512,2650,1353,2324,315,1022,
	2758,1308,2145,2559,1085,1782,1432,2274,1692,2734,1890,2855,140,2846,351,2369,3022,909,359,411,
	2903,3047,50,1421,2217,707,2694,638,1351,2433,555,499,666,1641,2124,1504,1376,1978,1237,793,
	1224,592,2458,209,1487,540,1074,572,2473,2881,2674,1064,1988,2810,229,1093,2120,353,1021,2646,
	2133,853,691,886,1574,1195,2548,2206,1810,1320,2850,1966,2216,1496,2990,2849,19,2785,2796,2531,
	171,2125,1797,1262,3024,1628,1292,943,1899,133,1713,1230,215,1937,1578,2472,2569,832,1784,436,
	448,2041,2344,1094,2914,1372,232,2159,248,1389,505,1878,2953,2105,331,231,601,2793,840,1132,
	1256,291,760,1567,2867,651,868,859,854,1481,2224,265,1728,1764,2632,1090,264,757,1509,595,
	1427,941,43,993,2602,2456,15,1749,1087,2644,1851,1839,1754,2034,816,1261,797,2138,2386,1648,
	2615,2619,663,388,119,2001,2927,163,210,1221,960,667,1682,365,2864,2697,1384,2626,2317,2915,
	624,1884,1060,779,1274,1684,1027,2905,2102,2208,1312,3020,13,1992,811,1651,286,908,2044,958,
	1537,695,2931,401,1781,1615,3048,2809,1434,3033,2541,2783,1668,2051,2033,1336,2009,973,2151,1136,
	2207,2402,823,1267,996,764,620,2022,2963,1191,2744,1395,2021,1216,2617,208,3031,1379,2221,580,
	1218,2863,783,1249,1284,1077,1202,1175,711,1275,449,190,1382,477,1447,2404,2652,2204,1138,2283,
	2654,703,2173,2198,2309,1413,2711,528,530,2935,2671,1629,598,1671,773,2365,1700,1483,581,1796,
	0,964,1360,2245,786,2010,2773,1258,1032,780,704,1895,1055,728,144,914,2121,693,1630,1977,
	2577,242,873,2321,2994,2004,1743,1758,1044,801,573,734,1940,1521,1996,1679,2032,594,1257,1775,
	1588,2675,2635,1059,1852,26,1248,247,872,880,395,2278,199,970,2966,2593,1846,1234,297,1417,
	1002,1607,460,30,2816,875,2640,2400,1836,905,1715,1970,2434,640,665,1540,1337,1564,2869,1942,
	2575,52,871,512,181,1850,2005,2409,1909,1554,1494,3041,792,2543,2808,1065,422,2672,90,1726,
	1908,1113,1824,1723,176,2449,2419,1530,1062,154,2109,2106,72,330,74,3042,824,1371,2585,616,
	492,2900,230,2587,2578,2177,2946,3052,2683,325,822,774,175,423,35,583,1265,564,1338,747,
	1994,1507,1991,2595,1171,2366,758,2943,137,126,225,273,746,3037,1605,183,440,316,178,1342,
	1959,2394,496,588,2786,38,258,1876,635,1283,479,1246,1956,1418,64,2230,1676,1349,3043,2231,
	1107,2144,751,930,2759,596,2187,2896,14,1789,2367,1495,290,2081,2906,259,1491,1179,2844,2061,
	266,2891,1573,1898,2252,2122,1531,1602,1116,626,1677,2094,929,924,335,2532,2647,2293,2079,414,
	2494,432,211,34,262,2770,1149,992,117,2938,1914,2921,1841,3045,2147,660,2964,2058,1549,1822,
	402,2572,1477,1239,2281,1048,1698,2840,893,1807,740,1123,1721,223,2378,1345,1460,919,1164,2842,
	1192,1596,2289,54,70,756,800,122,1515,1070,1738,1667,1355,387,1476,1367,2852,1637,2535,1159,
	2240,2624,3056,2383,2924,2684,1924,969,2141,2503,2712,2967,939,1319,1174,2414,462,883,355,2517,
	180,654,1456,2678,2848,2291,2608,1354,2558,814,1952,2157,1464,770,3026,2955,877,900,754,1717,
	408,952,474,2213,1786,571,1368,468,971,2865,1170,968,1146,1941,1957,2561,2689,2981,184,1816,
	652,2956,2811,1015,997,298,1752,1861,1105,1845,2930,478,112,818,251,3014,980,981,2405,458,
	417,1147,1652,896,2553,1933,1560,2600,2708,2062,972,2790,1444,1832,2310,338,1003,2074,1454,2288,
	819,426,367,825,127,1935,1709,244,109,979,2737,633,398,1999,2857,2718,1594,111,255,219,
	1210,412,1635,431,1831,1910,2438,2521,1885,193,3039,3006,1546,2014,1506,2099,2295,1916,2460,2082,
	237,322,2384,1982,1975,2847,715,1946,669,1660,2039,1213,1538,2060,2761,6,1331,2285,2779,2226,
	238,2570,2555,2152,847,1231,1470,910,2997,2766,1405,1244,2140,888,2579,1900,765,1891,1346,2132,
	1770,1387,2854,2667,913,1416,1099,849,2637,2529,1646,1645,485,2999,1765,656,1767,2160,149,130,
	2268,1080,1067,634,1518,182,809,1142,1235,121,2824,3003,1415,2016,2780,2174,699,1268,2453,2872,
	1948,1114,1965,2522,2150,1468,2700,1805,1095,2111,116,3071,466,803,1820,256,2534,2205,79,1938,
	534,2856,1654,305,607,1874,2645,2045,830,2692,2957,3049,1037,2660,65,156,1745,870,2391,1882,
	1241,1843,587,7,2170,378,547,2605,2028,1386,2477,2508,2934,2030,95,2011,2447,1410,421,1655,
	1499,18,2314,2625,2420,2649,2950,582,88,1068,739,2225,1199,2018,429,907,313,1245,2387,986,
	1902,113,2091,2653,2622,2104,2975,1109,1662,2524,222,1155,2439,749,1462,308,1196,2991,1057,2639,
	1193,2339,3002,2581,404,1040,2340,1897,118,2978,662,110,2886,869,2363,1467,1162,491,1838,1945,
	608,288,2024,1701,2706,1842,1759,2685,1120,2909,454,68,364,1203,1939,672,189,487,1503,923,
	442,1344,2498,1963,3012,680,2984,1306,1167,2129,25,899,2965,311,533,392,2002,1934,2973,2423,
	1286,1291,1756,1243,174,2481,1798,658,2214,3067,2566,2190,24,2398,2347,934,2126,2598,605,726,
	2095,2513,1580,228,2871,1347,1144,834,1896,1206,386,1226,2668,2665,318,906,2636,1130,2878,2399,
	2123,469,2330,2670,2942,2723,302,2248,956,1862,1119,1989,441,621,1401,2179,2565,556,2337,2073,
	815,1525,105,2707,1172,2725,81,817,637,798,1227,2229,1544,2807,2944,2236,2212,2354,611,1173,
	3040,1238,10,1276,1597,1928,1802,1364,2270,1609,2107,2186,2050,1394,420,280,927,1194,1097,123,
	1445,2590,2657,1917,1971,2519,1293,1129,537,2263,2441,49,1735,920,895,2395,2301,406,51,1614,
	188,2648,439,935,2113,2851,1683,299,418,2627,1887,1563,554,863,1603,323,396,959,3057,2341,
	2003,2853,839,106,2739,1696,73,284,2467,2452,2379,2031,1323,2717,2389,66,1812,2682,1539,2907,
	1561,1818,891,1576,2063,904,1181,1997,947,3061,950,1180,1694,2720,1497,1626,1840,1408,1045,1704,
	552,2629,1025,2370,2348,954,1465,150,1458,1613,2327,1835,1011,2279,250,2158,2373,2149,1620,1834,
	2294,2491,372,706,513,1339,1532,62,2286,766,527,1708,2163,2114,12,2609,864,2201,861,1673,
	1139,1719,2444,2266,2870,2078,56,2222,2320,368,2261,718,1814,3068,990,312,1829,2196,1984,76,
	3,2056,2161,1632,1747,358,2234,833,277,1359,1208,2688,1103,1913,1047,921,848,1979,285,214,
	2551,1930,1279,1639,260,2130,778,1672,1795,1729,1215,791,2537,827,1990,2136,93,2841,532,1302,
	2751,390,1335,1763,2432,1823,2722,1166,1746,2716,2153,2253,1330,1381,1761,2054,560,2825,1034,1472,
	2462,1052,1315,717,2988,1533,1392,2845,1043,1779,2454,142,1333,2883,37,1844,1869,2573,1143,1633,
	2920,2421,647,1358,2437,922,2874,1811,1653,221,170,2318,1611,2899,2742,733,2592,1955,3035,2833,
	671,541,1650,2071,2448,789,2486,710,590,1220,2827,1998,2515,2430,1236,994,1141,2902,1606,3030,
	2162,1849,2168,27,529,1806,549,2792,2422,1266,1209,1176,451,1748,220,2443,516,1285,1556,918,
	1592,2968,2574,1985,1550,926,664,1253,807,352,1774,688,272,2620,2582,1742,593,9,887,3007,
	1177,363,1282,2118,2726,989,1903,2932,2750,1356,998,1644,1642,1736,878,1690,1608,1555,2485,1259,
	1649,539,1278,134,761,1054,974,2116,1326,379,399,799,86,985,2638,650,932,287,1091,2815,
	1815,391,1987,2374,984,1623,1663,2765,2080,3059,138,1404,143,1833,2396,57,1450,2787,678,1145,
	2656,162,2745,2755,1522,2940,828,2424,856,104,1493,736,3053,2828,2416,3017,78,1794,1008,646,
	47,2131,862,1707,2949,785,1981,2064,1334,419,2797,1281,653,1622,1565,821,2455,347,1486,153,
	2643,1423,148,2312,2043,2052,300,67,141,11,397,2292,1128,1750,2784,2612,609,559,2687,623,
	1894,434,2446,804,2641,2731,2238,2819,376,2603,1640,2242,2112,98,103,2506,2393,2560,2563,2487,
	2335,452,1926,2360,128,2059,151,639,320,114,44,2584,424,1134,2686,345,2308,2098,2777,2669,
	281,1681,2721,2823,1082,1751,2331,2658,2631,2588,1905,2012,2093,1904,279,450,456,2311,1056,1773,
	425,536,2156,2868,2223,762,1950,1200,2936,3036,1479,1081,389,2812,446,2980,729,577,963,714,
	1826,2703,1297,476,21,2710,1595,558,1380,2552,1010,191,1769,2258,293,218,160,2549,1535,1451,
	91,2859,933,902,1513,1182,1362,1625,2418,681,3001,2267,2713,236,725,2583,1964,1572,1388,1714,
	890,603,2350,2699,2904,1088,2892,89,1598,2839,1821,1233,2948,2256,2743,2591,1030,1393,1616,289,
	2007,2343,2952,2415,1228,2403,1766,1046,204,928,599,1621,2818,2889,235,1974,2693,962,719,1311,
	570,1420,2961,1385,3004,860,1441,636,29,744,2019,2673,1689,1446,2568,2209,1951,304,772,1791,
	987,2184,2974,1102,1866,428,898,274,2020,1078,2053,2613,1014,1666,202,166,177,58,879,2610,
	1967,1552,548,333,1459,1466,1734,610,1510,205,2998,1161,2368,240,642,145,2461,2544,2382,735,
	2090,1272,1207,1471,1217,727,2188,3054,483,2417,1296,2730,775,881,1309,2300,1096,94,3016,2691,
	493,1352,852,782,1788,1300,2084,1361,1870,32,2589,720,473,579,164,1433,1363,1041,192,1049,
	2227,1,2210,600,2346,2047,2303,2775,2172,173,2505,497,453,2026,692,503,1548,659,82,1168,
	243,1819,1295,648,2408,509,955,2425,2679,2724,2200,2873,2069,430,1073,278,1101,470,409,295,
	1856,59,342,1343,2479,296,3009,1071,2076,1321,531,1739,876,2979,1972,813,1370,1868,1557,2338,
	2492,1636,936,2233,1520,1490,2092,201,2086,2760,2680,2199,1425,2835,1039,777,1288,1960,1732,1348,
	1001,2042,2435,2246,1280,1618,1855,1857,2250,107,1643,120,1986,1277,2514,942,310,1201,3046,1303,
	31,1140,213,2495,2476,897,1585,2747,1925,97,2838,2276,1502,1932,1787,2910,2377,2738,2664,1958,
	234,983,1911,1571,525,2908,366,698,2912,2469,1426,519,2297,2072,829,2571,2392,1601,2048,1893,
	1706,951,48,2539,2985,1264,2781,781,1214,2778,1674,2353,125,99,2919,2325,1438,20,2249,1122,
	569,844,2545,931,2376,2480,2947,1250,87,3062,1106,1198,1402,866,1799,1273,2511,615,2191,1328,
	2412,2326,1755,69,1524,348,544,1263,100,631,2530,697,769,1365,2597,159,1197,2709,2049,350,
	1205,2154,1702,45,545,1558,937,341,55,1137,2862,661,1877,2096,2146,1901,16,2315,2328,632,
	953,1665,835,1659,212,602,755,2893,340,2655,2630,1919,2829,628,1000,2546,2728,1443,2764,543,
	925,836,668,1076,1695,1398,197,249,2025,1036,1492,131,2428,753,2431,1419,1579,2244,22,2375,
	1111,521,2489,2475,2735,771,2763,1590,2406,1817,2029,2614,461,709,1624,2466,1430,2040,2837,2937,
	752,995,966,2677,894,2623,721,3064,2197,2576,1436,1063,1422,2719,165,2342,241,2243,196,892,
	1983,471,157,327,2385,2006,2596,677,394,2497,2538,685,2800,504,1875,655,2127,795,1439,562,
	136,722,147,1589,1657,2490,1473,1069,1442,2939,1718,1860,1830,80,550,912,1969,2926,1883,488,
	1638,790,2287,1186,2169,1185,1290,75,2804,2616,1304,3060,314,346,1686,344,946,1498,1482,1929,
	2464,940,2898,357,1669,1889,1920,1722,1604,198,538,2356,2768,2257,2358,2913,2798,2271,2788,328,
	1409,515,2101,2501,1872,1104,788,2795,1488,1813,657,203,2533,2542,2307,1954,2690,625,1553,542,
	1029,1658,670,472,233,1075,510,2525,1711,915,1511,309,362,467,2000,2929,2251,1923,161,1031,
	375,805,841,1307,2137,464,1514,2611,2860,270,1150,641,2027,1474,553,585,1251,2594,102,2749,
	53,563,1968,3015,991,306,1699,495,957,1369,480,696,2468,2065,373,1577,3051,2333,1771,2621,
	3018,567,978,2642,2265,1448,1294,332,1995,742,1976,2993,1867,168,2445,2822,2038,217,1131,565,
	1391,1163,2166,2733,2164,2371,1720,2971,1212,1247,763,1204,1753,949,2791,2951,2774,1357,1154,2547,
	1800,2500,2488,185,831,1329,3032,2933,1301,2746,2089,682,1169,326,584,2262,1661,403,1886,975,
	486,1219,275,2254,179,1455,1242,5,2928,1562,1403,1038
};
uint16_t log_transmission_sequence_cntr = 0;

/**
  * Returns most recent track point witch is complete.
  */
trackPoint_t* getLastTrackPoint(void)
{
	return lastTrackPoint;
}

void getNextLogTrackPoint(trackPoint_t* log)
{
	// Determine sector
	uint32_t address;
	do {
		// Determine which log has to be sent
		uint16_t log_id = log_transmission_sequence[log_transmission_sequence_cntr];

		// Determine sector and address
		if(log_id < LOG_SECTOR_SIZE/sizeof(trackPoint_t)) { // Flash sector 10
			address = LOG_FLASH_ADDR1 + log_id * sizeof(trackPoint_t);
		} else { // Flash sector 11
			address = LOG_FLASH_ADDR2 + log_id * sizeof(trackPoint_t) - LOG_SECTOR_SIZE;
		}

		// Increment sequence counter
		log_transmission_sequence_cntr = (log_transmission_sequence_cntr+1) % (sizeof(log_transmission_sequence) / sizeof(uint16_t));

	/* While the sequence has more values than the log has logs, we check if we
	 * are inside the log-address-range. The Sequence has more IDs because it
	 * would be possible to decrease sizeof(trackPoint_t) and then more Ids
	 * would be needed.
	 */
	} while(address > LOG_FLASH_ADDR2+LOG_SECTOR_SIZE);

	// Read data from memory
	flashRead(address, (char*)log, sizeof(trackPoint_t));
}

/**
  * Returns next free log entry address in memory. Returns 0 if all cells are
  * filled with data
  */
static uint32_t getNextFreeLogAddress(void)
{
	// Search in flash sector 10
	for(uint32_t address = LOG_FLASH_ADDR1; address < LOG_FLASH_ADDR1+LOG_SECTOR_SIZE; address += sizeof(trackPoint_t))
	{
		trackPoint_t pt;
		flashRead(address, (char*)&pt, sizeof(trackPoint_t));
		if(pt.id == 0xFFFFFFFF)
			return address;
	}

	// Search in flash sector 11
	for(uint32_t address = LOG_FLASH_ADDR2; address < LOG_FLASH_ADDR2+LOG_SECTOR_SIZE; address += sizeof(trackPoint_t))
	{
		trackPoint_t pt;
		flashRead(address, (char*)&pt, sizeof(trackPoint_t));
		if(pt.id == 0xFFFFFFFF)
			return address;
	}
	return 0;
}

/**
  * Returns next free log entry address in memory. Returns 0 if all cells are
  * filled with data
  */
static bool getLastLog(trackPoint_t* last)
{
	uint32_t last_id = 0;
	uint32_t last_address = 0;

	// Search in flash sector 10
	for(uint32_t address = LOG_FLASH_ADDR1; address < LOG_FLASH_ADDR1+LOG_SECTOR_SIZE; address += sizeof(trackPoint_t))
	{
		trackPoint_t pt;
		flashRead(address, (char*)&pt, sizeof(trackPoint_t));
		if(pt.id != 0xFFFFFFFF && pt.id >= last_id) {
			last_address = address;
			last_id = pt.id;
		}
	}

	// Search in flash sector 11
	for(uint32_t address = LOG_FLASH_ADDR2; address < LOG_FLASH_ADDR2+LOG_SECTOR_SIZE; address += sizeof(trackPoint_t))
	{
		trackPoint_t pt;
		flashRead(address, (char*)&pt, sizeof(trackPoint_t));
		if(pt.id != 0xFFFFFFFF && pt.id >= last_id) {
			last_address = address;
			last_id = pt.id;
		}
	}

	if(last_address) {
		flashRead(last_address, (char*)&last, sizeof(trackPoint_t));
		return true;
	} else {
		return false;
	}
}

/**
  * Erases oldest data
  */
static void eraseOldestLogData(void)
{
	// Determine which sector holds the oldest data
	trackPoint_t pt1, pt2;
	flashRead(LOG_FLASH_ADDR1, (char*)&pt1, sizeof(trackPoint_t));
	flashRead(LOG_FLASH_ADDR2, (char*)&pt2, sizeof(trackPoint_t));

	if(pt1.id < pt2.id) // Erase sector 10
	{
		TRACE_INFO("TRAC > Erase flash %08x", LOG_FLASH_ADDR1);
		flashErase(LOG_FLASH_ADDR1, LOG_SECTOR_SIZE);
	} else { // Erase sector 11
		TRACE_INFO("TRAC > Erase flash %08x", LOG_FLASH_ADDR2);
		flashErase(LOG_FLASH_ADDR2, LOG_SECTOR_SIZE);
	}
}

static void writeLogTrackPoint(trackPoint_t* tp)
{
	// Get address to write on
	uint32_t address = getNextFreeLogAddress();
	if(!address) // Memory completly used, erase oldest data
	{
		eraseOldestLogData();
		address = getNextFreeLogAddress();
	}
	if(!address) // Something went wront at erasing the memory
	{
		TRACE_ERROR("TRAC > Erasing flash failed");
		return;
	}

	// Write data into flash
	TRACE_INFO("TRAC > Flash write (ADDR=%08x)", address);
	flashSectorBegin(flashSectorAt(address));
	flashWrite(address, (char*)&tp, sizeof(trackPoint_t));
	flashSectorEnd(flashSectorAt(address));

	// Verify
	if(flashCompare(address, (char*)&tp, sizeof(trackPoint_t)))
		TRACE_INFO("TRAC > Flash write OK")
	else
		TRACE_ERROR("TRAC > Flash write failed");
}

void waitForNewTrackPoint(void)
{
	uint32_t old_id = getLastTrackPoint()->id;
	while(old_id == getLastTrackPoint()->id)
		chThdSleepMilliseconds(1000);
}

/**
  * Tracking Module (Thread)
  */
THD_FUNCTION(trackingThread, arg) {
	(void)arg;

	uint32_t id = 1;
	lastTrackPoint = &trackPoints[0]; // FIXME: That might not work

	// Fill initial values by PAC1720 and BME280 and RTC

	// Time
	ptime_t rtc;
	getTime(&rtc);
	lastTrackPoint->time.year = rtc.year;
	lastTrackPoint->time.month = rtc.month;
	lastTrackPoint->time.day = rtc.day;
	lastTrackPoint->time.hour = rtc.hour;
	lastTrackPoint->time.minute = rtc.minute;
	lastTrackPoint->time.second = rtc.second;

	// Get last GPS fix from memory
	trackPoint_t lastLogPoint;
	if(getLastLog(&lastLogPoint)) { // If there has been stored a trackpoint, then get the last know GPS fix
		lastTrackPoint->gps_lat = lastLogPoint.gps_lat;
		lastTrackPoint->gps_lon = lastLogPoint.gps_lon;
		lastTrackPoint->gps_alt = lastLogPoint.gps_alt;
	}

	lastTrackPoint->gps_lock = 0; // But tell the user that there is no current lock nor any GPS sats locked
	lastTrackPoint->gps_sats = 0;
	lastTrackPoint->gps_ttff = 0;

	// Debug last stored GPS position
	if(lastLogPoint.id != 0xFFFFFFFF) {
		TRACE_INFO(
			"TRAC > Last GPS position (from memory)\r\n"
			"%s Latitude: %d.%07ddeg\r\n"
			"%s Longitude: %d.%07ddeg\r\n"
			"%s Altitude: %d Meter",
			TRACE_TAB, lastTrackPoint->gps_lat/10000000, (lastTrackPoint->gps_lat > 0 ? 1:-1)*lastTrackPoint->gps_lat%10000000,
			TRACE_TAB, lastTrackPoint->gps_lon/10000000, (lastTrackPoint->gps_lon > 0 ? 1:-1)*lastTrackPoint->gps_lon%10000000,
			TRACE_TAB, lastTrackPoint->gps_alt
		);
	} else {
		TRACE_INFO("TRAC > No GPS position in memory");
	}

	// Voltage/Current
	lastTrackPoint->adc_vsol = getSolarVoltageMV();
	lastTrackPoint->adc_vbat = getBatteryVoltageMV();
	lastTrackPoint->adc_vusb = getUSBVoltageMV();
	lastTrackPoint->adc_psol = pac1720_getPsol();
	lastTrackPoint->adc_pbat = pac1720_getPbat();

	bme280_t bme280;

	// Atmosphere condition
	if(BME280_isAvailable(BME280_ADDRESS_INT)) {
		BME280_Init(&bme280, BME280_ADDRESS_INT);
		lastTrackPoint->air_press = BME280_getPressure(&bme280, 256);
		lastTrackPoint->air_hum = BME280_getHumidity(&bme280);
		lastTrackPoint->air_temp = BME280_getTemperature(&bme280);
	} else { // No internal BME280 found
		TRACE_ERROR("TRAC > No BME280 found");
		lastTrackPoint->air_press = 0;
		lastTrackPoint->air_hum = 0;
		lastTrackPoint->air_temp = 0;
	}

	systime_t time = chVTGetSystemTimeX();
	while(true)
	{
		TRACE_INFO("TRAC > Do module TRACKING MANAGER cycle");
		trac_conf.wdg_timeout = chVTGetSystemTimeX() + S2ST(600); // TODO: Implement more sophisticated method

		trackPoint_t* tp = &trackPoints[id % (sizeof(trackPoints) / sizeof(trackPoint_t))]; // Current track point
		trackPoint_t* ltp = &trackPoints[(id-1) % (sizeof(trackPoints) / sizeof(trackPoint_t))]; // Last track point

		// Search for GPS satellites
		gpsFix_t gpsFix = {{0,0,0,0,0,0,0},0,0,0,0,0};

		// Switch on GPS is enough power is available
		uint16_t batt = getBatteryVoltageMV();
		if(batt >= GPS_ON_VBAT)
		{
			// Switch on GPS
			GPS_Init();

			// Search for lock as long enough power is available
			do {
				batt = getBatteryVoltageMV();
				gps_get_fix(&gpsFix);
			} while(!isGPSLocked(&gpsFix) && batt >= GPS_OFF_VBAT && chVTGetSystemTimeX() <= time + S2ST(TRACK_CYCLE_TIME-5)); // Do as long no GPS lock and within timeout, timeout=cycle-1sec (-1sec in order to keep synchronization)

			if(batt < GPS_OFF_VBAT) // Switch off GPS at low batt
				GPS_Deinit();
		}

		if(isGPSLocked(&gpsFix)) { // GPS locked

			// Switch off GPS
			GPS_Deinit();

			// Debug
			TRACE_INFO("TRAC > GPS sampling finished GPS LOCK");
			TRACE_GPSFIX(&gpsFix);

			// Calibrate RTC
			setTime(gpsFix.time);

			// Take time from GPS
			tp->time.year = gpsFix.time.year;
			tp->time.month = gpsFix.time.month;
			tp->time.day = gpsFix.time.day;
			tp->time.hour = gpsFix.time.hour;
			tp->time.minute = gpsFix.time.minute;
			tp->time.second = gpsFix.time.second;

			// Set new GPS fix
			tp->gps_lat = gpsFix.lat;
			tp->gps_lon = gpsFix.lon;
			tp->gps_alt = gpsFix.alt;

			tp->gps_lock = isGPSLocked(&gpsFix);
			tp->gps_sats = gpsFix.num_svs;

		} else { // GPS lost (keep GPS switched on)

			// Debug
			TRACE_WARN("TRAC > GPS sampling finished GPS LOSS");

			// Take time from internal RTC
			getTime(&rtc);
			tp->time.year = rtc.year;
			tp->time.month = rtc.month;
			tp->time.day = rtc.day;
			tp->time.hour = rtc.hour;
			tp->time.minute = rtc.minute;
			tp->time.second = rtc.second;

			// Take GPS fix from old lock
			tp->gps_lat = ltp->gps_lat;
			tp->gps_lon = ltp->gps_lon;
			tp->gps_alt = ltp->gps_alt;

			// Mark gpsloss
			tp->gps_lock = false;
			tp->gps_sats = 0;

		}

		tp->id = id; // Serial ID
		tp->gps_ttff = ST2S(chVTGetSystemTimeX() - time); // Time to first fix

		// Power management
		tp->adc_vsol = getSolarVoltageMV();
		tp->adc_vbat = getBatteryVoltageMV();
		tp->adc_vusb = getUSBVoltageMV();
		tp->adc_psol = pac1720_getAvgPsol();
		tp->adc_pbat = pac1720_getAvgPbat();

		bme280_t bme280;

		// Atmosphere condition
		if(BME280_isAvailable(BME280_ADDRESS_INT)) {
			BME280_Init(&bme280, BME280_ADDRESS_INT);
			tp->air_press = BME280_getPressure(&bme280, 256);
			tp->air_hum = BME280_getHumidity(&bme280);
			tp->air_temp = BME280_getTemperature(&bme280);
		} else { // No internal BME280 found
			TRACE_ERROR("TRAC > Internal BME280 not available");
			tp->air_press = 0;
			tp->air_hum = 0;
			tp->air_temp = 0;
		}

		// Trace data
		TRACE_INFO(	"TRAC > New tracking point available (ID=%d)\r\n"
					"%s Time %04d-%02d-%02d %02d:%02d:%02d\r\n"
					"%s Pos  %d.%07d %d.%07d Alt %dm\r\n"
					"%s Sats %d  TTFF %dsec\r\n"
					"%s ADC Vbat=%d.%03dV Vsol=%d.%03dV VUSB=%d.%03dV Pbat=%dmW Psol=%dmW\r\n"
					"%s AIR p=%6d.%01dPa T=%2d.%02ddegC phi=%2d.%01d%%",
					tp->id,
					TRACE_TAB, tp->time.year, tp->time.month, tp->time.day, tp->time.hour, tp->time.minute, tp->time.day,
					TRACE_TAB, tp->gps_lat/10000000, (tp->gps_lat > 0 ? 1:-1)*tp->gps_lat%10000000, tp->gps_lon/10000000, (tp->gps_lon > 0 ? 1:-1)*tp->gps_lon%10000000, tp->gps_alt,
					TRACE_TAB, tp->gps_sats, tp->gps_ttff,
					TRACE_TAB, tp->adc_vbat/1000, (tp->adc_vbat%1000), tp->adc_vsol/1000, (tp->adc_vsol%1000), tp->adc_vusb/1000, (tp->adc_vusb%1000), tp->adc_pbat, tp->adc_psol,
					TRACE_TAB, tp->air_press/10, tp->air_press%10, tp->air_temp/100, tp->air_temp%100, tp->air_hum/10, tp->air_hum%10
		);

		// Append logging (timeout)
		if(nextLogEntryTimer <= chVTGetSystemTimeX() && isGPSLocked(&gpsFix))
		{
			writeLogTrackPoint(tp);
			nextLogEntryTimer += S2ST(LOG_CYCLE_TIME);
		}

		// Switch last recent track point
		lastTrackPoint = tp;
		id++;

		time = chThdSleepUntilWindowed(time, time + S2ST(TRACK_CYCLE_TIME)); // Wait until time + cycletime
	}
}

void init_tracking_manager(void)
{
	TRACE_INFO("TRAC > Startup tracking thread");
	thread_t *th = chThdCreateFromHeap(NULL, THD_WORKING_AREA_SIZE(2*1024), "TRA", NORMALPRIO, trackingThread, NULL);
	if(!th) {
		// Print startup error, do not start watchdog for this thread
		TRACE_ERROR("TRAC > Could not startup thread (not enough memory available)");
	} else {
		register_thread_at_wdg(&trac_conf);
		trac_conf.wdg_timeout = chVTGetSystemTimeX() + S2ST(1);
	}
}

