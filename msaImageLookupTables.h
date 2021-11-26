// lookup tables for bicubic interpolation, 64k based output, [0..255] input
int bcint1[] =
{
	0, -85, -169, -252, -333, -414, -494, -573, -651, -728, -804, -879, -953, -1026, -1098, -1170, 
	-1240, -1309, -1378, -1445, -1512, -1578, -1642, -1706, -1769, -1831, -1892, -1952, -2012, -2070, -2128, -2184, 
	-2240, -2295, -2349, -2402, -2454, -2506, -2556, -2606, -2655, -2703, -2750, -2797, -2842, -2887, -2931, -2974, 
	-3016, -3057, -3098, -3138, -3177, -3215, -3252, -3289, -3325, -3360, -3394, -3428, -3461, -3493, -3524, -3554, 
	-3584, -3613, -3641, -3669, -3695, -3721, -3747, -3771, -3795, -3818, -3840, -3862, -3883, -3903, -3923, -3942, 
	-3960, -3977, -3994, -4010, -4026, -4041, -4055, -4068, -4081, -4093, -4105, -4115, -4126, -4135, -4144, -4152, 
	-4160, -4167, -4173, -4179, -4184, -4189, -4193, -4196, -4199, -4201, -4203, -4204, -4204, -4204, -4203, -4202, 
	-4200, -4197, -4194, -4191, -4187, -4182, -4177, -4171, -4165, -4158, -4151, -4143, -4135, -4126, -4116, -4106, 
	-4096, -4085, -4074, -4062, -4049, -4036, -4023, -4009, -3995, -3980, -3965, -3949, -3933, -3916, -3899, -3882, 
	-3864, -3846, -3827, -3807, -3788, -3768, -3747, -3726, -3705, -3683, -3661, -3639, -3616, -3592, -3569, -3544, 
	-3520, -3495, -3470, -3444, -3418, -3392, -3365, -3338, -3311, -3283, -3255, -3227, -3198, -3169, -3140, -3110, 
	-3080, -3050, -3019, -2988, -2957, -2925, -2893, -2861, -2829, -2796, -2763, -2730, -2697, -2663, -2629, -2595, 
	-2560, -2525, -2490, -2455, -2419, -2384, -2348, -2311, -2275, -2238, -2201, -2164, -2127, -2090, -2052, -2014, 
	-1976, -1938, -1899, -1861, -1822, -1783, -1744, -1704, -1665, -1625, -1586, -1546, -1506, -1465, -1425, -1385, 
	-1344, -1303, -1262, -1221, -1180, -1139, -1098, -1056, -1015, -973, -932, -890, -848, -806, -764, -722, 
	-680, -638, -596, -553, -511, -468, -426, -384, -341, -298, -256, -213, -171, -128, -85, -43, 
};
int bcint2[] =
{
	65535, 65406, 65275, 65142, 65007, 64870, 64731, 64591, 64448, 64303, 64157, 64009, 63858, 63706, 63552, 63397, 
	63239, 63080, 62918, 62755, 62591, 62424, 62256, 62086, 61914, 61741, 61565, 61389, 61210, 61030, 60848, 60664, 
	60479, 60292, 60104, 59914, 59722, 59529, 59334, 59138, 58940, 58741, 58540, 58337, 58133, 57928, 57721, 57513, 
	57303, 57092, 56879, 56665, 56450, 56233, 56015, 55795, 55574, 55352, 55128, 54903, 54677, 54449, 54221, 53991, 
	53759, 53527, 53293, 53058, 52821, 52584, 52345, 52105, 51864, 51622, 51379, 51134, 50889, 50642, 50394, 50145, 
	49895, 49644, 49392, 49139, 48885, 48630, 48374, 48116, 47858, 47599, 47339, 47078, 46816, 46553, 46290, 46025, 
	45759, 45493, 45226, 44957, 44688, 44419, 44148, 43877, 43604, 43331, 43058, 42783, 42508, 42232, 41955, 41678, 
	41399, 41121, 40841, 40561, 40280, 39999, 39716, 39434, 39150, 38866, 38582, 38297, 38011, 37725, 37438, 37151, 
	36863, 36575, 36286, 35997, 35708, 35417, 35127, 34836, 34544, 34253, 33960, 33668, 33375, 33082, 32788, 32494, 
	32200, 31905, 31610, 31315, 31019, 30723, 30427, 30131, 29835, 29538, 29241, 28944, 28646, 28349, 28051, 27754, 
	27456, 27158, 26859, 26561, 26263, 25964, 25666, 25367, 25069, 24770, 24471, 24173, 23874, 23575, 23277, 22978, 
	22680, 22381, 22083, 21785, 21486, 21188, 20890, 20592, 20295, 19997, 19700, 19403, 19106, 18809, 18512, 18216, 
	17920, 17624, 17328, 17033, 16738, 16443, 16149, 15855, 15561, 15267, 14974, 14682, 14389, 14097, 13806, 13515, 
	13224, 12934, 12644, 12354, 12065, 11777, 11489, 11202, 10915, 10628, 10343, 10057, 9773, 9489, 9205, 8922, 
	8640, 8358, 8077, 7797, 7517, 7238, 6960, 6682, 6405, 6129, 5853, 5578, 5304, 5031, 4759, 4487, 
	4216, 3946, 3677, 3408, 3141, 2874, 2608, 2343, 2079, 1816, 1554, 1292, 1032, 772, 514, 256, 
};
int bcint3[] =
{
	0, 256, 514, 772, 1032, 1292, 1554, 1816, 2079, 2343, 2608, 2874, 3141, 3408, 3677, 3946, 
	4216, 4487, 4759, 5031, 5304, 5578, 5853, 6129, 6405, 6682, 6960, 7238, 7517, 7797, 8077, 8358, 
	8640, 8922, 9205, 9489, 9773, 10057, 10343, 10628, 10915, 11202, 11489, 11777, 12065, 12354, 12644, 12934, 
	13224, 13515, 13806, 14097, 14389, 14682, 14974, 15267, 15561, 15855, 16149, 16443, 16738, 17033, 17328, 17624, 
	17920, 18216, 18512, 18809, 19106, 19403, 19700, 19997, 20295, 20592, 20890, 21188, 21486, 21785, 22083, 22381, 
	22680, 22978, 23277, 23575, 23874, 24173, 24471, 24770, 25069, 25367, 25666, 25964, 26263, 26561, 26859, 27158, 
	27456, 27754, 28051, 28349, 28646, 28944, 29241, 29538, 29835, 30131, 30427, 30723, 31019, 31315, 31610, 31905, 
	32200, 32494, 32788, 33082, 33375, 33668, 33960, 34253, 34544, 34836, 35127, 35417, 35708, 35997, 36286, 36575, 
	36863, 37151, 37438, 37725, 38011, 38297, 38582, 38866, 39150, 39434, 39716, 39999, 40280, 40561, 40841, 41121, 
	41399, 41678, 41955, 42232, 42508, 42783, 43058, 43331, 43604, 43877, 44148, 44419, 44688, 44957, 45226, 45493, 
	45759, 46025, 46290, 46553, 46816, 47078, 47339, 47599, 47858, 48116, 48374, 48630, 48885, 49139, 49392, 49644, 
	49895, 50145, 50394, 50642, 50889, 51134, 51379, 51622, 51864, 52105, 52345, 52584, 52821, 53058, 53293, 53527, 
	53759, 53991, 54221, 54449, 54677, 54903, 55128, 55352, 55574, 55795, 56015, 56233, 56450, 56665, 56879, 57092, 
	57303, 57513, 57721, 57928, 58133, 58337, 58540, 58741, 58940, 59138, 59334, 59529, 59722, 59914, 60104, 60292, 
	60479, 60664, 60848, 61030, 61210, 61389, 61565, 61741, 61914, 62086, 62256, 62424, 62591, 62755, 62918, 63080, 
	63239, 63397, 63552, 63706, 63858, 64009, 64157, 64303, 64448, 64591, 64731, 64870, 65007, 65142, 65275, 65406, 
};
int bcint4[] =
{
	0, -43, -85, -128, -171, -213, -256, -298, -341, -384, -426, -468, -511, -553, -596, -638, 
	-680, -722, -764, -806, -848, -890, -932, -973, -1015, -1056, -1098, -1139, -1180, -1221, -1262, -1303, 
	-1344, -1385, -1425, -1465, -1506, -1546, -1586, -1625, -1665, -1704, -1744, -1783, -1822, -1861, -1899, -1938, 
	-1976, -2014, -2052, -2090, -2127, -2164, -2201, -2238, -2275, -2311, -2348, -2384, -2419, -2455, -2490, -2525, 
	-2560, -2595, -2629, -2663, -2697, -2730, -2763, -2796, -2829, -2861, -2893, -2925, -2957, -2988, -3019, -3050, 
	-3080, -3110, -3140, -3169, -3198, -3227, -3255, -3283, -3311, -3338, -3365, -3392, -3418, -3444, -3470, -3495, 
	-3520, -3544, -3569, -3592, -3616, -3639, -3661, -3683, -3705, -3726, -3747, -3768, -3788, -3807, -3827, -3846, 
	-3864, -3882, -3899, -3916, -3933, -3949, -3965, -3980, -3995, -4009, -4023, -4036, -4049, -4062, -4074, -4085, 
	-4096, -4106, -4116, -4126, -4135, -4143, -4151, -4158, -4165, -4171, -4177, -4182, -4187, -4191, -4194, -4197, 
	-4200, -4202, -4203, -4204, -4204, -4204, -4203, -4201, -4199, -4196, -4193, -4189, -4184, -4179, -4173, -4167, 
	-4160, -4152, -4144, -4135, -4126, -4115, -4105, -4093, -4081, -4068, -4055, -4041, -4026, -4010, -3994, -3977, 
	-3960, -3942, -3923, -3903, -3883, -3862, -3840, -3818, -3795, -3771, -3747, -3721, -3695, -3669, -3641, -3613, 
	-3584, -3554, -3524, -3493, -3461, -3428, -3394, -3360, -3325, -3289, -3252, -3215, -3177, -3138, -3098, -3057, 
	-3016, -2974, -2931, -2887, -2842, -2797, -2750, -2703, -2655, -2606, -2556, -2506, -2454, -2402, -2349, -2295, 
	-2240, -2184, -2128, -2070, -2012, -1952, -1892, -1831, -1769, -1706, -1642, -1578, -1512, -1445, -1378, -1309, 
	-1240, -1170, -1098, -1026, -953, -879, -804, -728, -651, -573, -494, -414, -333, -252, -169, -85, 
};


// this returns a cosine curve, scaled for 64k, for the input range of [0 .. 255] for use in cosine interpolation
const int fastCos[] = 
{
	65535, 65533, 65525, 65513, 65496, 65473, 65446, 65414, 65377, 65335, 65289, 65237, 65180,
	65119, 65053, 64981, 64905, 64825, 64739, 64648, 64553, 64453, 64348, 64238, 64124, 64005,
	63881, 63753, 63620, 63482, 63339, 63192, 63041, 62885, 62724, 62559, 62389, 62215, 62036,
	61853, 61666, 61474, 61278, 61078, 60873, 60664, 60451, 60234, 60013, 59787, 59558, 59324,
	59087, 58845, 58600, 58351, 58097, 57840, 57580, 57315, 57047, 56775, 56500, 56220, 55938,
	55652, 55362, 55069, 54773, 54474, 54171, 53865, 53555, 53243, 52927, 52609, 52287, 51963,
	51636, 51306, 50973, 50637, 50298, 49957, 49614, 49268, 48919, 48568, 48214, 47859, 47501,
	47140, 46778, 46413, 46047, 45678, 45308, 44935, 44561, 44185, 43807, 43428, 43047, 42664,
	42280, 41895, 41508, 41119, 40730, 40339, 39948, 39555, 39161, 38766, 38370, 37974, 37576,
	37178, 36779, 36380, 35980, 35580, 35179, 34778, 34376, 33974, 33572, 33170, 32768, 32366,
	31964, 31562, 31160, 30759, 30358, 29957, 29557, 29157, 28757, 28358, 27960, 27563, 27166,
	26771, 26376, 25982, 25589, 25197, 24806, 24417, 24029, 23642, 23256, 22872, 22490, 22109,
	21729, 21351, 20975, 20601, 20229, 19858, 19490, 19123, 18758, 18396, 18036, 17678, 17322,
	16968, 16617, 16269, 15922, 15579, 15238, 14899, 14564, 14231, 13900, 13573, 13249, 12927,
	12609, 12293, 11981, 11671, 11365, 11062, 10763, 10466, 10173, 9884, 9598, 9315, 9036,
	8761, 8489, 8221, 7956, 7695, 7438, 7185, 6936, 6690, 6449, 6211, 5978, 5748,
	5523, 5301, 5084, 4871, 4662, 4457, 4257, 4061, 3869, 3682, 3499, 3320, 3146,
	2976, 2811, 2650, 2494, 2343, 2195, 2053, 1915, 1782, 1654, 1530, 1411, 1296,
	1187, 1082, 982, 886, 796, 710, 629, 553, 482, 415, 354, 297, 246,
	199, 157, 120, 88, 61, 39, 21, 9, 1 
};

// inverse of above
const int fastCosInv[] = 
{
	0, 2, 10, 22, 39, 62, 89, 121, 158, 200, 246, 298, 355,
	416, 482, 554, 630, 710, 796, 887, 982, 1082, 1187, 1297, 1411, 1530,
	1654, 1782, 1915, 2053, 2196, 2343, 2494, 2650, 2811, 2976, 3146, 3320, 3499,
	3682, 3869, 4061, 4257, 4457, 4662, 4871, 5084, 5301, 5522, 5748, 5977, 6211,
	6448, 6690, 6935, 7184, 7438, 7695, 7955, 8220, 8488, 8760, 9035, 9315, 9597,
	9883, 10173, 10466, 10762, 11061, 11364, 11670, 11980, 12292, 12608, 12926, 13248, 13572,
	13899, 14229, 14562, 14898, 15237, 15578, 15921, 16267, 16616, 16967, 17321, 17676, 18034,
	18395, 18757, 19122, 19488, 19857, 20227, 20600, 20974, 21350, 21728, 22107, 22488, 22871,
	23255, 23640, 24027, 24416, 24805, 25196, 25587, 25980, 26374, 26769, 27165, 27561, 27959,
	28357, 28756, 29155, 29555, 29955, 30356, 30757, 31159, 31561, 31963, 32365, 32767, 33169,
	33571, 33973, 34375, 34776, 35177, 35578, 35978, 36378, 36778, 37177, 37575, 37972, 38369,
	38764, 39159, 39553, 39946, 40338, 40729, 41118, 41506, 41893, 42279, 42663, 43045, 43426,
	43806, 44184, 44560, 44934, 45306, 45677, 46045, 46412, 46777, 47139, 47499, 47857, 48213,
	48567, 48918, 49266, 49613, 49956, 50297, 50636, 50971, 51304, 51635, 51962, 52286, 52608,
	52926, 53242, 53554, 53864, 54170, 54473, 54772, 55069, 55362, 55651, 55937, 56220, 56499,
	56774, 57046, 57314, 57579, 57840, 58097, 58350, 58599, 58845, 59086, 59324, 59557, 59787,
	60012, 60234, 60451, 60664, 60873, 61078, 61278, 61474, 61666, 61853, 62036, 62215, 62389,
	62559, 62724, 62885, 63041, 63192, 63340, 63482, 63620, 63753, 63881, 64005, 64124, 64239,
	64348, 64453, 64553, 64649, 64739, 64825, 64906, 64982, 65053, 65120, 65181, 65238, 65289,
	65336, 65378, 65415, 65447, 65474, 65496, 65514, 65526, 65534
};

