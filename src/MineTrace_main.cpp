//
//  Author: Jonathan Decker
//  Decription: Renders an 8x8 grid of chunks (16x16x128 blocks) of a Minecraft World.
//  The user can shift in other chunks with the WASD keys, and change the density.
//  Modified from NVIDIA sdk example for rendering a volume stored in a 3D texture.
//  Uses getenv_s to find the APPDATA folder.
//
//  usage: <MineTrace> <World Number>
//
//  Controls:
//      c       - Toggle drawing the extents of the volume in wireframe\n");
//      g       - Toggle drawing the extents of chunks in wireframe\n");
//      l       - Toggle rendering with opacity lighting \n");
//      p       - shift chunk with player position \n");
//   [ and ]    - Change density\n");
//   ; and '    - Change brightness\n");
//   , and .    - Change alpha for non-ore\n");
//   w and s    - Shift map in z dimension\n");
//   a and d    - Shift map in x dimension\n");
//
////////////////////////////////////////////////////////////////////////////////

#include <vld.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <stack>
#include <map>
#include <string>
#include <windows.h>
#include <assert.h>

#include <GL/glew.h>
#include <GL/glut.h>
#include <Cg/cg.h>

#include "blocks.hpp"

extern "C"
{
    #include "nbt.h"
}

#include "nvGlutManipulators.h"
#include "VolumeRender.h"

#define LO(w)           ((BYTE)(((DWORD_PTR)(w)) & 0xf))
#define HI(w)           ((BYTE)((((DWORD_PTR)(w)) >> 4) & 0xf))

using std::map;

enum UIOption {
    OPTION_DRAW_CUBE,
	OPTION_DRAW_CHUNKS,
    OPTION_COUNT
};
bool options[OPTION_COUNT];
map<char, UIOption> optionKeyMap;

#ifdef FULLSCREEN
int width = 1920;
int height = 1080;
#else
int width = 1024;
int height = 768;
#endif

int world = 1;
int cx = 19;
int cz = -19;
float viewDistance = -4.0f;
int spawnx;
int spawnz;
bool retrievedSpawn = false;
LARGE_INTEGER start_time;
LARGE_INTEGER time_freq;
//unsigned char blockArr[32768];
//unsigned char radiArr[16384];

nv::GlutExamine trackball;

CGcontext cgContext; 
VolumeRender * volumeRender = NULL;
VolumeBuffer * vBuff = NULL;
ImageBuffer * cBuff = NULL;
unsigned char* vData = NULL;
int gWin = -1;
bool alphaLight = false;

float density = 1.0f;
float nonOreAlpha = 1.0f;
float brightness = 1.0f;
mc::color BlockC[255];

char base32digits[37] = "0123456789abcdefghijklmnopqrstuvwxyz";


std::string base36(int M)
{	
	std::stack<short> intStack;
	while(M >= 36)
	{	
		intStack.push(M % 36);
		M = M/36;
	}	
	std::string str;
	str.append(base32digits + M, 1);
	while(!intStack.empty())
	{
		str.append(base32digits + intStack.top(), 1);
	    intStack.pop();
	}
	return str;	
}

void CleanUp()
{
	if( gWin != -1)
	    glutDestroyWindow(gWin);
	if( vBuff != NULL )
		delete vBuff;
	if( volumeRender != NULL )
		delete volumeRender;
	if( vData != NULL )
		delete[] vData;
	if( cBuff != NULL )
		delete cBuff;

	mc::deinitialize_constants();
}

void text_output(GLfloat x, GLfloat y, GLfloat z, char *text)
{
    char *p;

	glRasterPos3f(x, y, z);
	for (p = text; *p; p++)
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *p);
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    trackball.applyTransform();

    // render volume
    glViewport(0, 0, width, height);
    volumeRender->render();

	glEnable(GL_LINE_SMOOTH);

	glColor3f(1.0,1.0,1.0);

	text_output(-0.55, 0.55, -0.55, "X");
	text_output(-0.55, -0.55, 0.55, "Z");
	text_output(0.55, 0.55, 0.55, "Y");

	if (options[OPTION_DRAW_CHUNKS])
	{
		glTranslatef(0.0,0.5,0.5);
		glBegin(GL_LINES);

		glVertex3f(0.5,0.0,0.0);
		glVertex3f(0.5,-1.0,0.0);

		glVertex3f(0.5,0.0,0.0);
		glVertex3f(0.5,0.0,-1.0);

		glVertex3f(-0.5,0.0,0.0);
		glVertex3f(-0.5,-1.0,0.0);

		glVertex3f(-0.5,0.0,0.0);
		glVertex3f(-0.5,0.0,-1.0);

		for(unsigned int r = 0; r < 9; r++)
		{
			glVertex3f(-0.5,-0.125*r,0.0);
			glVertex3f(0.5,-0.125*r,0.0);
		}
		for(unsigned int r = 1; r < 9; r++)
		{
			glVertex3f(-0.5,0.0,-0.125*r);
			glVertex3f(0.5,0.0,-0.125*r);
		}

		// bottom grid
		for(unsigned int r = 0; r < 9; r++)
		{
			glVertex3f(-0.5,-0.125*r,0.0);
			glVertex3f(-0.5,-0.125*r,-1.0);
		}
		for(unsigned int r = 1; r < 9; r++)
		{
			glVertex3f(-0.5,0.0,-0.125*r);
			glVertex3f(-0.5,-1.0,-0.125*r);
		}

		// upper grid
		for(unsigned int r = 0; r < 9; r++)
		{
			glVertex3f(0.5,-0.125*r,0.0);
			glVertex3f(0.5,-0.125*r,-1.0);
		}
		for(unsigned int r = 1; r < 9; r++)
		{
			glVertex3f(0.5,0.0,-0.125*r);
			glVertex3f(0.5,-1.0,-0.125*r);
		}
		glEnd();
	}
    else if (options[OPTION_DRAW_CUBE])
		glutWireCube(1.0f);
	

    glutSwapBuffers();
}

void mouse( int button, int state, int x, int y)
{
    trackball.mouse(button, state, x, y);

    glutPostRedisplay();
}

void motion(int x, int y)
{
    trackball.motion(x, y);
    glutPostRedisplay();
}

void idle()
{
	LARGE_INTEGER curr_time;
	QueryPerformanceCounter(&curr_time);
	int diff = (int)((float)((curr_time.QuadPart - start_time.QuadPart)/(double)time_freq.QuadPart) * 1000);

	if( diff % 20 == 0 )
	{
		trackball.idle();
	}

    glutPostRedisplay();
}

void reshape(int w, int h)
{
    width = w;
    height = h;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(20.0, (double)width / height, 0.1, 10.0);
    
    glViewport(0, 0, width, height);

    trackball.reshape(w, h);
}

// we require OpenGL 2.0 or greater
void initGL()
{
    glewInit();
    if (!glewIsSupported( "GL_VERSION_2_0 GL_EXT_framebuffer_object" )) {
        printf( "Required extensions not supported.\n");
		CleanUp();
        exit(-1);
    }

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
    //glEnable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	//glEnable(GL_ALPHA_TEST);
	//glAlphaFunc(GL_GREATER, 0.01f);
}

// error callback method for cg toolkit errors
void cgErrorCallback()
{
    CGerror lastError = cgGetError();
    if(lastError)
    {
        printf("%s\n", cgGetErrorString(lastError));
        printf("%s\n", cgGetLastListing(cgContext));
		CleanUp();
        exit(1);
    }
}

// determines the name of the chunk file for the chunk at position (x,y)
void chunkFile( char* str, int x, int y )
{
	// This implementation allows for negative modular results
	int xt = x % 64;
	int yt = y % 64;

	if( xt < 0 ) xt += 64;
	if( yt < 0 ) yt += 64;

	std::string xBlock = base36( xt );
	std::string yBlock = base36( yt );
	std::string x36 = base36( abs(x) );
	std::string y36 = base36( abs(y) );

	if( x < 0 ) x36 = "-" + x36;
	if( y < 0 ) y36 = "-" + y36;

	std::string temp = xBlock + "\\" + yBlock + "\\c." + x36 + "." + y36 + ".dat";

	strcpy(str,temp.c_str());
}

// determines the name of the region file for the chunk at position (x,y)
void regionFile( char* str, int x, int y )
{
   sprintf(str,"r.%d.%d.mcr", x>>5, y>>5 );
}

mc::color GetColor(unsigned char c)
{
	mc::color temp = mc::MaterialColor[c];
	if( c != mc::RedstoneOre &&
		c != mc::GlowingRedstoneOre &&
		c != mc::DiamondOre &&
		c != mc::GoldOre &&
		c != mc::IronOre &&
		c != mc::CoalOre &&
		c != mc::LapisLazuliOre )
		temp.a *= nonOreAlpha;
	return temp;
}

void InitColors()
{
	BlockC[0] = mc::color(255,255,255,0);
	BlockC[1] = mc::color(120,120,120,255);
	BlockC[2] = mc::color(117,176,73,255);
	BlockC[3] = mc::color(134,96,67,255);
	BlockC[4] = mc::color(115,115,115,255);
	BlockC[48] = mc::color(115,115,115,255);
	BlockC[5] = mc::color(157,128,79,255);
	BlockC[6] = mc::color(120,120,120,0);
	BlockC[7] = mc::color(84,84,84,255);
	BlockC[8] = mc::color(38,92,255,51);
	BlockC[9] = mc::color(38,92,255,51);
	BlockC[10] = mc::color(255,90,0,255);
	BlockC[11] = mc::color(255,90,0,255);
	BlockC[12] = mc::color(218,210,158,255);
	BlockC[13] = mc::color(136,126,126,255);
	BlockC[14] = mc::color(143,140,125,255);
	BlockC[15] = mc::color(136,130,127,255);
	BlockC[16] = mc::color(115,115,115,255);
	BlockC[17] = mc::color(102,81,51,255);
	BlockC[18] = mc::color(60,192,41,100);
	BlockC[20] = mc::color(255,255,255,64); //glass
	//BlockC[21] = mc::color(222,50,50,255);
	//BlockC[22] = mc::color(222,136,50,255);
	//BlockC[23] = mc::color(222,222,50,255);
	//BlockC[24] = mc::color(136,222,50,255);
	//BlockC[25] = mc::color(50,222,50,255);
	//BlockC[26] = mc::color(50,222,136,255);
	//BlockC[27] = mc::color(50,222,222,255);
	//BlockC[28] = mc::color(104,163,222,255);
	//BlockC[29] = mc::color(120,120,222,255);
	//BlockC[30] = mc::color(136,50,222,255);
	//BlockC[31] = mc::color(174,74,222,255);
	//BlockC[32] = mc::color(222,50,222,255);
	//BlockC[33] = mc::color(222,50,136,255);
	//BlockC[34] = mc::color(77,77,77,255);
	BlockC[35] = mc::color(222,222,222,255); //mc::color(143,143,143,255); 
	//BlockC[36] = mc::color(222,222,222,255);
	BlockC[38] = mc::color(255,0,0,255);
	BlockC[37] = mc::color(255,255,0,255);
	BlockC[41] = mc::color(231,165,45,255);
	BlockC[42] = mc::color(191,191,191,255);
	BlockC[43] = mc::color(200,200,200,255);
	BlockC[44] = mc::color(200,200,200,255);
	BlockC[45] = mc::color(170,86,62,255);
	BlockC[46] = mc::color(160,83,65,255);
	BlockC[49] = mc::color(26,11,43,255);
	BlockC[50] = mc::color(245,220,50,200);
	BlockC[51] = mc::color(255,170,30,200);
	//BlockC[52] = mc::color(245,220,50,255); unnecessary afaik
	BlockC[53] = mc::color(157,128,79,255);
	BlockC[54] = mc::color(125,91,38,255);
	//BlockC[55] = mc::color(245,220,50,255); unnecessary afaik
	BlockC[56] = mc::color(129,140,143,255);
	BlockC[57] = mc::color(45,166,152,255);
	BlockC[58] = mc::color(114,88,56,255);
	BlockC[59] = mc::color(146,192,0,255);
	BlockC[60] = mc::color(95,58,30,255);
	BlockC[61] = mc::color(96,96,96,255);
	BlockC[62] = mc::color(96,96,96,255);
	BlockC[63] = mc::color(111,91,54,255);
	BlockC[64] = mc::color(136,109,67,255);
	BlockC[65] = mc::color(181,140,64,32);
	BlockC[66] = mc::color(150,134,102,180);
	BlockC[67] = mc::color(115,115,115,255);
	BlockC[71] = mc::color(191,191,191,255);
	BlockC[73] = mc::color(131,107,107,255);
	BlockC[74] = mc::color(131,107,107,255);
	BlockC[75] = mc::color(181,140,64,32);
	BlockC[76] = mc::color(255,0,0,200);
	BlockC[78] = mc::color(255,255,255,255);
	BlockC[79] = mc::color(83,113,163,51);
	BlockC[80] = mc::color(250,250,250,255);
	BlockC[81] = mc::color(25,120,25,255);
	BlockC[82] = mc::color(151,157,169,255);
	BlockC[83] = mc::color(193,234,150,255);
	BlockC[83] = mc::color(100,67,50,255);
}

// read in 8x8 grid of chunks, starting from provided top-left position
int ReadMineCraftOldFormat( unsigned char* data, unsigned int w,
				            unsigned int bw, unsigned int bh,
				            unsigned int bxs = 0, unsigned int bys = 0,
				            unsigned int bxe = 0, unsigned int bye = 0,
				            bool useSpawn = false)
{
	nbt_file* nbt = NULL;
	char base[80];
	char path[256];
	char chunk[256];
	unsigned int chunkmax = 16 * 16 * 128;

	bool chunkFound = false;

	if (nbt_init(&nbt) != NBT_OK)
    {
        fprintf(stderr, "NBT_Init(): Failure initializing\n");
        return 0;
    }

	unsigned int len;
    getenv_s(&len, base, 80, "APPDATA");

	// Check if world data exists
	sprintf(base, "%s\\.minecraft\\saves\\World%d", base, w );

	WIN32_FIND_DATA ffd;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	
	hFind = FindFirstFile( base, &ffd );
    if (hFind == INVALID_HANDLE_VALUE) 
	{
		printf("Cannot find World %d directory\n", w);
		nbt_free(nbt);
		return 0;
    } 
	FindClose(hFind);
	hFind = INVALID_HANDLE_VALUE;

	// get spawn point from level.dat

	if( !retrievedSpawn )
	{
		retrievedSpawn = true;
		sprintf(path, "%s\\level.dat", base);
		if (nbt_parse(nbt, path,0) == NBT_OK)
		{
			nbt_tag *data = nbt_find_tag_by_name("Data", nbt->root);
			//nbt_tag *spawn = nbt_find_tag_by_name("SpawnX", data);

			//spawnx = *nbt_cast_int(spawn) / 16;
			//spawn = nbt_find_tag_by_name("SpawnZ", data);
			//spawnz = *nbt_cast_int(spawn) / 16;

			nbt_tag *player = nbt_find_tag_by_name("Player", data);
			nbt_tag *pos = nbt_find_tag_by_name( "Pos", player);
			nbt_list* posList = nbt_cast_list(pos);
			double** posArr = (double**)posList -> content;

			spawnx = (int)*posArr[0] / 16;
			spawnz = (int)*posArr[2] / 16;

			nbt_free_tag(nbt->root);
			nbt->root = NULL;
		}
	}

	if( useSpawn )
	{
		cx = spawnx;
		cz = spawnz;
	}

	// load chunks
	for( unsigned int i = bxs; i < bw - bxe; i++)
	{
		for( unsigned int j = bys; j < bh - bye; j++)
		{
			chunkFile(chunk,cx + i,cz + j);

			sprintf(path, "%s\\%s", base, chunk);

			if (nbt_parse(nbt, path, 0) != NBT_OK)
			{
				printf("No chunk at (%d,%d)\n",cx+i,cz+j);

				// zero out missing chunk
				for(unsigned int x = 0; x < 16; x++)
				{
					for(unsigned int z = 0; z < 16; z++)
					{
						for(unsigned int y = 0; y < 128; y++)
						{
							unsigned int texPos = (y + (j * 16 + z) * 128 + (i * 16 + x) * 128 * 128) * 4;

							if( texPos > chunkmax * 4 * 64 )
								printf("Error: Bad position in target texture\n");
							else
							{
								data[texPos] = 0;
								data[texPos+1] = 0;
								data[texPos+2] = 0;
								data[texPos+3] = 0;
							}
						}
					}
				}

				continue;
			}
			printf("Reading chunk at (%d,%d)...\n",cx+i,cz+j);

			// write chunk colors to float array

			//nbt_compound* rootNode = nbt_cast_compound(nbt->root);
			nbt_tag *level = nbt_find_tag_by_name("Level", nbt->root);
            nbt_tag *blocks = nbt_find_tag_by_name("Blocks", level);
			nbt_tag *sky = nbt_find_tag_by_name("SkyLight", level);
			nbt_tag *radi = nbt_find_tag_by_name("BlockLight", level);
			if( blocks == NULL || sky == NULL || radi == NULL )
			{
				nbt_free_tag(nbt->root);
				nbt->root = NULL;
				continue;
			}
			nbt_byte_array* blockArr = nbt_cast_byte_array(blocks);
			nbt_byte_array* skyArr = nbt_cast_byte_array(sky);
			nbt_byte_array* radiArr = nbt_cast_byte_array(radi);
			if( blockArr == NULL || skyArr == NULL || radiArr == NULL )
			{
				nbt_free_tag(nbt->root);
				nbt->root = NULL;
				continue;
			}

			// Write blocks to texture	
			for(unsigned int x = 0; x < 16; x++)
			{
				for(unsigned int z = 0; z < 16; z++)
				{
					for(unsigned int y = 0; y < 128; y++)
					{
						unsigned int texPos = (y + (j * 16 + z) * 128 + (i * 16 + x) * 128 * 128) * 4;

						if( texPos > chunkmax * 4 * 64 )
							printf("Error: Bad position in target texture\n");
						else
						{
							unsigned int bpos = y + z * 128 + x * 128 * 16;
							unsigned char c = blockArr->content[ bpos ];
							mc::color bCol = BlockC[c];
							unsigned char sc = skyArr->content[ bpos / 2 ];
							unsigned char rc = radiArr->content[ bpos / 2 ];

							if( bpos % 2 == 0 )
							{
								sc = LO(sc);
								rc = LO(rc);
							}
							else
							{
								sc = HI(sc);
								rc = HI(rc);
							}

							//sc * 17;
							//rc * 17;

							float d = (sc / 15.0f);
							float r = (rc / 15.0f);

							d += r + 0.50f;
						    if( d > 1 ) d = 1;

							data[texPos] = (char)(((bCol.r / 255.0f) * d) * 255.0f);
							data[texPos+1] = (char)(((bCol.g / 255.0f) * d) * 255.0f);
							data[texPos+2] = (char)(((bCol.b / 255.0f) * d) * 255.0f);
							data[texPos+3] = (char)((bCol.a / 255.0f) * 255.0f * (alphaLight ? d : 1.0f));

							//data[texPos] = (char)rc;
							//data[texPos+1] = (char)c;
							//data[texPos+2] = (char)sc;
						}
					}
				}
			}
			nbt_free_tag(nbt->root);
			nbt->root = NULL;
		}
	}

	nbt_free(nbt);

	return 1;
}

const int CHUNK_DEFLATE_MAX = 1024 * 64;  // 64KB limit for compressed chunks
const int CHUNK_INFLATE_MAX = 1024 * 128; // 128KB limit for inflated chunks

unsigned char in[CHUNK_DEFLATE_MAX], out[CHUNK_INFLATE_MAX];

// read in 8x8 grid of chunks, starting from provided top-left position
int ReadMineCraft( unsigned char* data, unsigned int w,
				   unsigned int bw, unsigned int bh,
				   unsigned int bxs = 0, unsigned int bys = 0,
				   unsigned int bxe = 0, unsigned int bye = 0,
				   bool useSpawn = false)
{
	nbt_file* nbt = NULL;
	char base[80];
	char path[256];
	char region[256];
	unsigned int chunkmax = 16 * 16 * 128;

	bool chunkFound = false;

	if (nbt_init(&nbt) != NBT_OK)
    {
        fprintf(stderr, "NBT_Init(): Failure initializing\n");
        return 0;
    }

	unsigned int len;
    getenv_s(&len, base, 80, "APPDATA");

	// Check if world data exists
	sprintf(base, "%s\\.minecraft\\saves\\World%d", base, w );

	WIN32_FIND_DATA ffd;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	
	hFind = FindFirstFile( base, &ffd );
    if (hFind == INVALID_HANDLE_VALUE) 
	{
		printf("Cannot find World %d directory\n", w);
		nbt_free(nbt);
		return 0;
    } 
	FindClose(hFind);
	hFind = INVALID_HANDLE_VALUE;

	// get spawn point from level.dat

	if( !retrievedSpawn )
	{
		retrievedSpawn = true;
		sprintf(path, "%s\\level.dat", base);
		if (nbt_parse(nbt, path,0) == NBT_OK)
		{
			nbt_tag *data = nbt_find_tag_by_name("Data", nbt->root);
			//nbt_tag *spawn = nbt_find_tag_by_name("SpawnX", data);

			//spawnx = *nbt_cast_int(spawn) / 16;
			//spawn = nbt_find_tag_by_name("SpawnZ", data);
			//spawnz = *nbt_cast_int(spawn) / 16;

			nbt_tag *player = nbt_find_tag_by_name("Player", data);
			nbt_tag *pos = nbt_find_tag_by_name( "Pos", player);
			nbt_list* posList = nbt_cast_list(pos);
			double** posArr = (double**)posList -> content;

			spawnx = (int)*posArr[0] / 16;
			spawnz = (int)*posArr[2] / 16;

			nbt_free_tag(nbt->root);
			nbt->root = NULL;
		}
	}

	if( useSpawn )
	{
		cx = spawnx;
		cz = spawnz;
	}

	//unsigned char* skyArr = nbt_cast_byte_array(sky);

	// load chunks
	for( unsigned int i = bxs; i < bw - bxe; i++)
	{
		for( unsigned int j = bys; j < bh - bye; j++)
		{
			regionFile(region,cx + i,cz + j);
			//chunkFile(chunk,cx + i,cz + j);

			sprintf(path, "%s\\region\\%s", base, region);

			int chunkOffset, chunkLength;
			//chunkPos(path, cx + i,cz + j, &chunkOffset, &chunkLength);

			unsigned char buf[5];
			int sectorNumber;
			bool readOk = false;
			z_stream strm;
			int status;
			int pos = 4 * (((cx + i) & 31) + ((cz + j) & 31) * 32);
			

			chunkOffset = 0;

			FILE* ptr = fopen(path, "rb");
			if( ptr != NULL )
			{
			   if( fseek(ptr,pos,SEEK_SET) == 0 )
			   {
				   if( fread(buf, 4, 1, ptr) == 1 )
				   {
					   sectorNumber = buf[3];
					   chunkOffset = buf[0]<<16 | buf[1]<<8 | buf[2];

					   if( chunkOffset != 0 )
					   {
						   fseek(ptr, 4096*chunkOffset, SEEK_SET);
						   if( fread(buf, 5, 1, ptr) == 1 )
						   {
							   chunkLength = buf[0]<<24 | buf[1]<<16 | buf[2]<<8 | buf[3];

							   if( (chunkLength <= sectorNumber * 4096) && (chunkLength <= CHUNK_DEFLATE_MAX) && (buf[4] == 2) )
							   {
								   if( fread(in, chunkLength - 1, 1, ptr) == 1 )
								   {
									   fclose(ptr);

										// decompress chunk
										strm.zalloc = (alloc_func)NULL;
										strm.zfree = (free_func)NULL;
										strm.opaque = NULL;

										strm.next_out = out;
										strm.avail_out = CHUNK_INFLATE_MAX;
										strm.avail_in = chunkLength - 1;
										strm.next_in = in;

										inflateInit(&strm);
										status = inflate(&strm, Z_FINISH); // decompress in one step
										inflateEnd(&strm);

										if (status == Z_STREAM_END) 
										{
											if( nbt_parse_buffer(nbt, out, strm.avail_out) == NBT_OK )
											{
												readOk = true;
											}
										}
								   }
								   else fclose(ptr);
							   }
							   else fclose(ptr);
						   }
						   else fclose(ptr);
					   }
					   else fclose(ptr);
				   }
				   else fclose(ptr);
			   }
			   else fclose(ptr);
			}

			if( chunkOffset == 0 || !readOk )
			{
#ifdef _DEBUG
				printf("No chunk at (%d,%d)\n",cx+i,cz+j);
#endif
				// zero out missing chunk
				for(unsigned int x = 0; x < 16; x++)
				{
					for(unsigned int z = 0; z < 16; z++)
					{
						for(unsigned int y = 0; y < 128; y++)
						{
							unsigned int texPos = (y + (j * 16 + z) * 128 + (i * 16 + x) * 128 * 128) * 4;

							if( texPos > chunkmax * 4 * 64 )
								printf("Error: Bad position in target texture\n");
							else
							{
								data[texPos] = 0;
								data[texPos+1] = 0;
								data[texPos+2] = 0;
								data[texPos+3] = 0;
							}
						}
					}
				}

				continue;
			}
#ifdef _DEBUG
			printf("Reading chunk at (%d,%d)...\n",cx+i,cz+j);
#endif
			// write chunk colors to float array

			//nbt_compound* rootNode = nbt_cast_compound(nbt->root);
			nbt_tag *level = nbt_find_tag_by_name("Level", nbt->root);
            nbt_tag *blocks = nbt_find_tag_by_name("Blocks", level);
			nbt_tag *sky = nbt_find_tag_by_name("SkyLight", level);
			nbt_tag *radi = nbt_find_tag_by_name("BlockLight", level);
			if( blocks == NULL || sky == NULL || radi == NULL )
			{
				nbt_free_tag(nbt->root);
				nbt->root = NULL;
				continue;
			}
			nbt_byte_array* blockArr = nbt_cast_byte_array(blocks);
			nbt_byte_array* skyArr = nbt_cast_byte_array(sky);
			nbt_byte_array* radiArr = nbt_cast_byte_array(radi);
			if( blockArr == NULL || skyArr == NULL || radiArr == NULL )
			{
				nbt_free_tag(nbt->root);
				nbt->root = NULL;
				continue;
			}

			// Write blocks to texture	
			for(unsigned int x = 0; x < 16; x++)
			{
				for(unsigned int z = 0; z < 16; z++)
				{
					for(unsigned int y = 0; y < 128; y++)
					{
						unsigned int texPos = (y + (j * 16 + z) * 128 + (i * 16 + x) * 128 * 128) * 4;

						if( texPos > chunkmax * 4 * 64 )
						{
							printf("Error: Bad position in target texture\n");
						}
						else
						{
							unsigned int bpos = y + z * 128 + x * 128 * 16;
							unsigned char c = blockArr->content[ bpos ];
							mc::color bCol = GetColor(c);
							unsigned char sc = skyArr->content[ bpos / 2 ];
							unsigned char rc = radiArr->content[ bpos / 2 ];

							if( bpos % 2 == 0 )
							{
								sc = LO(sc);
								rc = LO(rc);
							}
							else
							{
								sc = HI(sc);
								rc = HI(rc);
							}

							float d = (sc / 15.0f);
							float r = (rc / 15.0f);

							d += r + 0.50f;
						    if( d > 1 ) d = 1;

							//float d = 1;

							data[texPos] = (char)(((bCol.r / 255.0f) * d) * 255.0f);
							data[texPos+1] = (char)(((bCol.g / 255.0f) * d) * 255.0f);
							data[texPos+2] = (char)(((bCol.b / 255.0f) * d) * 255.0f);
							data[texPos+3] = (char)((bCol.a / 255.0f) * 255.0f * (alphaLight ? d : 1.0f));

							//data[texPos] = (char)rc;
							//data[texPos+1] = (char)c;
							//data[texPos+2] = (char)sc;
						}
					}
				}
			}
			nbt_free_tag(nbt->root);
			nbt->root = NULL;
		}
	}

	nbt_free(nbt);

	return 1;
}

void shiftChunks( unsigned char* data, int X, int Z, unsigned int bw, unsigned int bh )
{
	unsigned int s = 128 * 128;
	if( X < 0 )
	{
		for( unsigned int y = 0; y < 128; y++)
		{
			for( unsigned int z = 0; z < 128; z++)
			{
				for( unsigned int x = 16; x < 128; x++)
				{
					unsigned int lp = y + z * 128 + (x-16) * s;
					unsigned int rp = y + z * 128 + x * s;
					data[lp*4] = data[rp*4];
					data[lp*4 + 1] = data[rp*4 + 1];
					data[lp*4 + 2] = data[rp*4 + 2];
					data[lp*4 + 3] = data[rp*4 + 3];
				}
			}
		}
	}
	else if( X > 0 )
	{
		for( unsigned int y = 0; y < 128; y++)
		{
			for( unsigned int z = 0; z < 128; z++)
			{
				for( int x = 111; x >= 0; x--)
				{
					unsigned int lp = y + z * 128 + (x+16) * s;
					unsigned int rp = y + z * 128 + x * s;
					data[lp*4] = data[rp*4];
					data[lp*4 + 1] = data[rp*4 + 1];
					data[lp*4 + 2] = data[rp*4 + 2];
					data[lp*4 + 3] = data[rp*4 + 3];
				}
			}
		}
	}

	if( Z < 0 )
	{
		for( unsigned int y = 0; y < 128; y++)
		{
			for( unsigned int x = 0; x < 128; x++)
			{
				for( unsigned int z = 16; z < 128; z++)
				{
					unsigned int lp = y + (z-16) * 128 + x * s;
					unsigned int rp = y + z * 128 + x * s;
					data[lp*4] = data[rp*4];
					data[lp*4 + 1] = data[rp*4 + 1];
					data[lp*4 + 2] = data[rp*4 + 2];
					data[lp*4 + 3] = data[rp*4 + 3];
				}
			}
		}
	}
	else if( Z > 0 )
	{
		for( unsigned int y = 0; y < 128; y++)
		{
			for( unsigned int x = 0; x < 128; x++)
			{
				for( int z = 111; z >= 0; z--)
				{
					unsigned int lp = y + (z+16) * 128 + x * s;
					unsigned int rp = y + z * 128 + x * s;
					data[lp*4] = data[rp*4];
					data[lp*4 + 1] = data[rp*4 + 1];
					data[lp*4 + 2] = data[rp*4 + 2];
					data[lp*4 + 3] = data[rp*4 + 3];
				}
			}
		}
	}
}

void ShiftWorld( unsigned char* data, unsigned int w, 
				 int x, int z, unsigned int bw, unsigned int bh )
{
	shiftChunks(data, x, z, bw, bh);
	ReadMineCraft(data,w,bw,bh,
		          x < 0 ? bw-2 : 0,
				  z < 0 ? bh-2 : 0,
				  x > 0 ? bw-1 : 0,
				  z > 0 ? bh-1 : 0 );
}

void key(unsigned char c, int x, int y)
{
    c = tolower(c);

    if (optionKeyMap.find(c) != optionKeyMap.end())
        options[optionKeyMap[c]] = ! options[optionKeyMap[c]];

    switch (c) {
        case '\033': //escape key
        case 'q':
			CleanUp();
            exit(0);
            break;
		case 'a': // left arrow
			++cx;
			ShiftWorld(vData, world, -1,  0, 8, 8);
			vBuff->setData(vData);
			break;
		case 'w': // up arrow
			--cz;
			ShiftWorld(vData, world, 0,  1, 8, 8);
			vBuff->setData(vData);
			break;
		case 'd': // right arrow
			--cx;
			ShiftWorld(vData, world, 1,  0, 8, 8);
			vBuff->setData(vData);
			break;
		case 's': // down arrow
			++cz;
			ShiftWorld(vData, world, 0, -1, 8, 8);
			vBuff->setData(vData);
			break;
		case 'l': // toggle light scan
			alphaLight = !alphaLight;
			ReadMineCraft(vData, world, 8, 8);
			vBuff->setData(vData);
			break;
		case '[':
			density -= 0.01f;
			if( density < 0 ) density = 0;
			volumeRender->setDensity(density);
			break;
		case ']':
			density += 0.01f;
			if( density > 1 ) density = 1;
			volumeRender->setDensity(density);
			break;
		case ';':
			brightness -= 0.01f;
			if( brightness < 0 ) brightness = 0;
			volumeRender->setBrightness(brightness);
			break;
		case '\'':
			brightness += 0.01f;
			if( brightness > 1 ) brightness = 1;
			volumeRender->setBrightness(brightness);
			break;
		case ',':
			nonOreAlpha -= 0.01f;
			if( nonOreAlpha < 0 ) nonOreAlpha = 0;
			ReadMineCraft(vData, world, 8, 8);
			vBuff->setData(vData);
			break;
		case '.':
			nonOreAlpha += 0.01f;
			if( nonOreAlpha > 1 ) nonOreAlpha = 1;
			ReadMineCraft(vData, world, 8, 8);
			vBuff->setData(vData);
			break;
		case 'p':
			ReadMineCraft(vData, world, 8, 8, 0, 0, 0, 0, true);
			vBuff->setData(vData);
			break;
    }

    glutPostRedisplay();
}

void mainMenu(int i)
{
    key((unsigned char) i, 0, 0);
}

void initMenus() {
    glutCreateMenu(mainMenu);
    glutAddMenuEntry("Toggle cube     [c]", 'c');
	glutAddMenuEntry("Toggle chunks   [g]", 'g');
	glutAddMenuEntry("Toggle alpha    [l]", 'l');
	glutAddMenuEntry("Move to player  [p]", 'p');
    glutAddMenuEntry("Quit          [esc]", '\033');
    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

int main(int argc, char** argv)
{
	world = 1;
	cx = 19;
	cz = -19;
	bool useSpawn = true;
	if( argc > 1 )
	{
		world = atoi(argv[1]);
		if( argc > 3 )
		{
			cx = atoi(argv[2]);
			cz = atoi(argv[3]);
			useSpawn = false;
		}
	}

	QueryPerformanceCounter(&start_time);
	QueryPerformanceFrequency(&time_freq);

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
    glutInitWindowSize(width, height);
    gWin = glutCreateWindow("MineTrace");

    glutDisplayFunc(display);
    glutKeyboardFunc(key);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutIdleFunc(idle);

    initGL();
    initMenus();

#ifdef FULLSCREEN
	glutFullScreen();
#endif
    trackball.setDollyActivate(GLUT_LEFT_BUTTON, GLUT_ACTIVE_CTRL);
    trackball.setPanActivate(GLUT_LEFT_BUTTON, GLUT_ACTIVE_SHIFT);
    trackball.setDollyPosition(viewDistance);

    // create simulation
    cgContext = cgCreateContext();
    cgSetErrorCallback(cgErrorCallback);

	
	vBuff = new VolumeBuffer(GL_RGBA16F_ARB, 128, 128, 128, 1);
	unsigned int size = 128*128*128*4;
	vData = new unsigned char[size];

	InitColors();
	mc::initialize_constants();
	cBuff = new ImageBuffer(GL_RGBA16F_ARB, 16, 16, 1);
	cBuff -> setData((unsigned char*)BlockC);

	// Get MineCraft Data
	int succ = ReadMineCraft( vData, world, 8, 8, 0, 0, 0, 0, useSpawn );
	if( succ )
	{
		vBuff -> setData(vData);
		volumeRender = new VolumeRender(cgContext, vBuff, cBuff);
		volumeRender->setDensity(density);
		volumeRender->setBrightness(brightness);

		//setup the option keys
		optionKeyMap['c'] = OPTION_DRAW_CUBE;
		options[OPTION_DRAW_CUBE] = false;
		optionKeyMap['g'] = OPTION_DRAW_CHUNKS;
		options[OPTION_DRAW_CHUNKS] = false;

		printf( "MineTrace - displaying 64 chunks of selected world\n");
		printf( "commandline arguements : MineTrace <world number> <NW chunk X> <NW chunk Z>" );
		printf( "   q/[ESC]    - Quit the app\n");
		printf( "      c       - Toggle drawing the extents of the volume in wireframe\n");
		printf( "      g       - Toggle drawing the extents of chunks in wireframe\n");
		printf( "      l       - Toggle rendering with opacity lighting \n");
		printf( "      p       - shift chunk with player position \n");
		printf( "   [ and ]    - Change density\n");
		printf( "   ; and '    - Change brightness\n");
		printf( "   , and .    - Change alpha for non-ore\n");
		printf( "   w and s    - Shift map in z dimension\n");
		printf( "   a and d    - Shift map in x dimension\n");
		
		glutMainLoop();
	}
	else
	{
		CleanUp();
	}

    return 0;
}
