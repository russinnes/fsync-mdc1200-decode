/*-
 * fsync_common.c
 * Author: Matthew Kaufman (matthew@eeph.com)
 *
 * Copyright (c) 2012, 2013, 2014  Matthew Kaufman  All rights reserved.
 * 
 *  This file is part of Matthew Kaufman's fsync Encoder/Decoder Library
 *
 *  The fsync Encoder/Decoder Library is free software; you can
 *  redistribute it and/or modify it under the terms of version 2 of
 *  the GNU General Public License as published by the Free Software
 *  Foundation.
 *
 *  If you cannot comply with the terms of this license, contact
 *  the author for alternative license arrangements or do not use
 *  or redistribute this software.
 *
 *  The fsync Encoder/Decoder Library is distributed in the hope
 *  that it will be useful, but WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 *  USA.
 *
 *  or see http://www.gnu.org/copyleft/gpl.html
 *
-*/


static float _sintable[] = {
	0.000000,  0.024541,  0.049068,  0.073565,  0.098017,  0.122411,  0.146730,  0.170962,
	0.195090,  0.219101,  0.242980,  0.266713,  0.290285,  0.313682,  0.336890,  0.359895,
	0.382683,  0.405241,  0.427555,  0.449611,  0.471397,  0.492898,  0.514103,  0.534998,
	0.555570,  0.575808,  0.595699,  0.615232,  0.634393,  0.653173,  0.671559,  0.689541,
	0.707107,  0.724247,  0.740951,  0.757209,  0.773010,  0.788346,  0.803208,  0.817585,
	0.831470,  0.844854,  0.857729,  0.870087,  0.881921,  0.893224,  0.903989,  0.914210,
	0.923880,  0.932993,  0.941544,  0.949528,  0.956940,  0.963776,  0.970031,  0.975702,
	0.980785,  0.985278,  0.989177,  0.992480,  0.995185,  0.997290,  0.998795,  0.999699,
	1.000000,  0.999699,  0.998795,  0.997290,  0.995185,  0.992480,  0.989177,  0.985278,
	0.980785,  0.975702,  0.970031,  0.963776,  0.956940,  0.949528,  0.941544,  0.932993,
	0.923880,  0.914210,  0.903989,  0.893224,  0.881921,  0.870087,  0.857729,  0.844854,
	0.831470,  0.817585,  0.803208,  0.788346,  0.773010,  0.757209,  0.740951,  0.724247,
	0.707107,  0.689541,  0.671559,  0.653173,  0.634393,  0.615232,  0.595699,  0.575808,
	0.555570,  0.534998,  0.514103,  0.492898,  0.471397,  0.449611,  0.427555,  0.405241,
	0.382683,  0.359895,  0.336890,  0.313682,  0.290285,  0.266713,  0.242980,  0.219101,
	0.195090,  0.170962,  0.146730,  0.122411,  0.098017,  0.073565,  0.049068,  0.024541,
	0.000000, -0.024541, -0.049068, -0.073565, -0.098017, -0.122411, -0.146730, -0.170962,
	-0.195090, -0.219101, -0.242980, -0.266713, -0.290285, -0.313682, -0.336890, -0.359895,
	-0.382683, -0.405241, -0.427555, -0.449611, -0.471397, -0.492898, -0.514103, -0.534998,
	-0.555570, -0.575808, -0.595699, -0.615232, -0.634393, -0.653173, -0.671559, -0.689541,
	-0.707107, -0.724247, -0.740951, -0.757209, -0.773010, -0.788346, -0.803208, -0.817585,
	-0.831470, -0.844854, -0.857729, -0.870087, -0.881921, -0.893224, -0.903989, -0.914210,
	-0.923880, -0.932993, -0.941544, -0.949528, -0.956940, -0.963776, -0.970031, -0.975702,
	-0.980785, -0.985278, -0.989177, -0.992480, -0.995185, -0.997290, -0.998795, -0.999699,
	-1.000000, -0.999699, -0.998795, -0.997290, -0.995185, -0.992480, -0.989177, -0.985278,
	-0.980785, -0.975702, -0.970031, -0.963776, -0.956940, -0.949528, -0.941544, -0.932993,
	-0.923880, -0.914210, -0.903989, -0.893224, -0.881921, -0.870087, -0.857729, -0.844854,
	-0.831470, -0.817585, -0.803208, -0.788346, -0.773010, -0.757209, -0.740951, -0.724247,
	-0.707107, -0.689541, -0.671559, -0.653173, -0.634393, -0.615232, -0.595699, -0.575808,
	-0.555570, -0.534998, -0.514103, -0.492898, -0.471397, -0.449611, -0.427555, -0.405241,
	-0.382683, -0.359895, -0.336890, -0.313682, -0.290285, -0.266713, -0.242980, -0.219101,
	-0.195090, -0.170962, -0.146730, -0.122411, -0.098017, -0.073565, -0.049068, -0.024541 };


static int _fsync_crc(int word1, int word2)
{

        int paritybit = 0;
        int crcsr = 0;
        int bit;
        int cur;
        int invert;

        for(bit = 0; bit < 48; bit++)
        {
                if(bit < 32)
                {
                    cur = (word1 >> (31-bit)) & 0x01;
                }
                else
                {
                    cur = (word2 >> (31-(bit-32))) & 0x01;
                }
                if(cur)
                    paritybit ^= 1;
                invert = cur ^ (0x01 & (crcsr>>15));
                if(invert)
                {
                        crcsr ^= 0x6815;
                }
                crcsr <<= 1;
        }

		for(bit = 48; bit<63; bit++)
		{
                cur = (word2 >> (31-(bit-32))) & 0x01;
                if(cur)
                    paritybit ^= 1;
		}

        crcsr ^= 0x0002;
        crcsr += paritybit;
        return crcsr & 0xffff;
}

static int pcheck[] = { 0x4045, 0x2067, 0x1076, 0x083b, 0x0458, 0x022c, 0x0116, 0x008b };
static int rpcheck[] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x17, 0x2e, 0x5c, 0xb8, 0x67, 0xce, 0x8b };

static int _fsync2_ecc_repair(int input)
{
	int result = 0;
	int i,j;
	int s;

	for(i=0; i<8; i++)
	{
		s = 0;
		for(j=0; j<15; j++)
		{
			s += ((pcheck[i] >> j) & 1) * ((input>>j) & 1);
		}
		if(s & 1)
		{
			result += 1<<i;
		}
	}

	if(result == 0)
		return input;

	for(i=0; i<15; i++)
	{
		if(result == rpcheck[i])
		{
			return input ^ (1<<(14-i));
		}
	}
	return -1;
}

