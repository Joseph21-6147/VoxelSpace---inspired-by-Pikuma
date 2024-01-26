// Voxel Space (Comanche Terrain Rendering) - inspired by Pikuma (thanks!)
// ========================================

// Youtube: https://youtu.be/bQBY9BM9g_Y
// demo version (can be bugged, no guarantees,...)
//
// Have fun with it!
// Joseph21
//
// January 26, 2024

#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

// keep the screen dimensions constant and vary the resolution by adapting the pixel size
#define SCREEN_X    700
#define SCREEN_Y    400
#define PIXEL_X       1
#define PIXEL_Y       1

// map width and height
#define MAP_N         1024
#define SCALE_FACTOR    70.0    // not sure what this is for

#define MAPS_AVAILABLE    30                   // number of maps provided
#define PATH_DATA_COLOR   "maps/color_data/"   // the locations where the files are
#define PATH_DATA_HEIGHT  "maps/height_data/"


// Camera class declaration
class camera_t {

public:
    float x, y;      // position on the map
    float height;    // height of the camera
    float horizon;   // offset of the horizon position (looking up-down)
    float zfar;      // distance of the camera looking forward
    float angle;     // camera angle (radians, clockwise)

public:
    camera_t() {}

    void Init( float _x, float _y, float _z, float _hor, float _far, float _a ) {
        x       = _x;
        y       = _y;
        height  = _z;
        horizon = _hor;
        zfar    = _far;
        angle   = _a;
    }
};

class VoxelSpace : public olc::PixelGameEngine {

public:
    VoxelSpace() {
        sAppName = "VoxelSpace (vid by Pikuma)";
        sAppName.append( " - S:(" + std::to_string( SCREEN_X / PIXEL_X ) + ", " + std::to_string( SCREEN_Y / PIXEL_Y ) + ")" );
        sAppName.append(  ", P:(" + std::to_string(            PIXEL_X ) + ", " + std::to_string(            PIXEL_Y ) + ")" );
    }

private:
    // Buffers for Heightmap and Colormap
    uint8_t     *heightmap = nullptr;   // Buffer/array to hold height values (1024*1024)
    olc::Sprite *heightspr = nullptr;
    olc::Sprite *colormap  = nullptr;   // texture pointer to hold color values  (1024*1024)

    // Camera
    camera_t camera;

    // convenience input functions - needed to easily page through the different maps
    void ReadSprite( const std::string &sPath, int nNr, olc::Sprite **res ) {
        if (*res != nullptr) {
            delete *res;
        }
        std::string sFullPath = sPath + "map";
        if (nNr < 10) sFullPath.append( "0" );
        sFullPath.append( std::to_string( nNr ) + ".png" );
        *res = new olc::Sprite( sFullPath );
        if (*res == nullptr || (*res)->width == 0 || (*res)->height == 0) {
            std::cout << "ERROR: ReadSprite() --> failuring reading file: " << sFullPath << std::endl;
        }
    }

    void ReadColorData( int nNr ) {
        ReadSprite( PATH_DATA_COLOR, nNr, &colormap );
    }

    /* the height data was provided in a gif file. I don't know how to parse gif, so I converted them to
     * png files that can be read as an olc::Sprite. It turns out that each of the r, g and b values then contains
     * the original height data (uint8_t).
     *
     * Having found this, I can convert the sprite into the desired heightmap using a little hack.
     */
    void ReadHeightData( int nNr ) {

        // read heightmap data into a sprite
        ReadSprite( PATH_DATA_HEIGHT, nNr, &heightspr );
        // create heightmap
        if (heightmap != nullptr)
            delete heightmap;

        heightmap = new uint8_t[ MAP_N * MAP_N ];
        // set heightmap using the sprite pixel data
        float exaggerate = 1.5f;
        for (int x = 0; x < MAP_N; x++) {
            for (int y = 0; y < MAP_N; y++) {
                heightmap[y * MAP_N + x] = uint8_t( exaggerate * float( heightspr->GetPixel( x, y ).r ));
            }
        }
    }

    // map management stuff

    int nActiveMap = 0;

    void ResetMapData() {
        ReadColorData(  nActiveMap );
        ReadHeightData( nActiveMap );
    }

    void MapNext() { nActiveMap = (nActiveMap + 1                 ) % MAPS_AVAILABLE; ResetMapData(); }
    void MapPrev() { nActiveMap = (nActiveMap - 1 + MAPS_AVAILABLE) % MAPS_AVAILABLE; ResetMapData(); }

public:
    bool OnUserCreate() override {

        // read map data (color and height)
        ResetMapData();
        // init the camera
        camera.Init(
            512.0f, 512.0f, 70.0f,    // x, y and height
            60.0f,                    // horizon
            600.0f,                   // zFar value
            1.5f * 3.1415926535f      // angle (= 270 deg)
        );

        return true;
    }

    bool OnUserUpdate( float fElapsedTime ) override {

        // page through the available maps
        if (GetKey( olc::Key::NP_ADD ).bPressed) { MapNext(); }
        if (GetKey( olc::Key::NP_SUB ).bPressed) { MapPrev(); }

        // set different speeds
        float fSpeedStrafe  = 20.0f;
        float fSpeedRotate  =  1.0f;
        float fAccellerator = fElapsedTime;
        if (GetKey( olc::Key::SHIFT).bHeld) fAccellerator *= 5.0f;
        if (GetKey( olc::Key::CTRL ).bHeld) fAccellerator *= 0.2f;

        // Handle controls - rotation
        if (GetKey( olc::Key::A    ).bHeld) { camera.angle   -= fAccellerator * fSpeedRotate; }
        if (GetKey( olc::Key::D    ).bHeld) { camera.angle   += fAccellerator * fSpeedRotate; }
        // elevation
        if (GetKey( olc::Key::UP   ).bHeld) { camera.height  += fAccellerator * fSpeedStrafe; }
        if (GetKey( olc::Key::DOWN ).bHeld) { camera.height  -= fAccellerator * fSpeedStrafe; }
        // horizon manipulation
        if (GetKey( olc::Key::PGUP ).bHeld) { camera.horizon += fAccellerator * fSpeedStrafe; }
        if (GetKey( olc::Key::PGDN ).bHeld) { camera.horizon -= fAccellerator * fSpeedStrafe; }

        float sinangle = sin( camera.angle );
        float cosangle = cos( camera.angle );
        // moving forward / aft
        if (GetKey(olc::Key::W).bHeld) { camera.x += fAccellerator * fSpeedStrafe * cosangle; camera.y += fAccellerator * fSpeedStrafe * sinangle; }
        if (GetKey(olc::Key::S).bHeld) { camera.x -= fAccellerator * fSpeedStrafe * cosangle; camera.y -= fAccellerator * fSpeedStrafe * sinangle; }
        // strafing left / right
        if (GetKey(olc::Key::Q).bHeld) { camera.x += fAccellerator * fSpeedStrafe * sinangle; camera.y -= fAccellerator * fSpeedStrafe * cosangle; }
        if (GetKey(olc::Key::E).bHeld) { camera.x -= fAccellerator * fSpeedStrafe * sinangle; camera.y += fAccellerator * fSpeedStrafe * cosangle; }

        // Pikuma logic below
        // ==================

        // Left-most point of the FOV
        float plx = cosangle * camera.zfar + sinangle * camera.zfar;
        float ply = sinangle * camera.zfar - cosangle * camera.zfar;

        // Right-most point of the FOV
        float prx = cosangle * camera.zfar - sinangle * camera.zfar;
        float pry = sinangle * camera.zfar + cosangle * camera.zfar;

        Clear( olc::BLACK );

        // Loop ScreenWidth() rays from left to right
        for (int i = 0; i < ScreenWidth(); i++) {
            float deltax = (plx + (prx - plx) / ScreenWidth() * i) / camera.zfar;
            float deltay = (ply + (pry - ply) / ScreenWidth() * i) / camera.zfar;

            // Ray (x,y) coords
            float rx = camera.x;
            float ry = camera.y;

            // Store the tallest projected height per-ray
            float tallestheight = ScreenHeight();

            // Loop all depth units until the zfar distance limit
            for (int z = 1; z < camera.zfar; z++) {
                rx += deltax;
                ry += deltay;

                // Find the offset that we have to go and fetch values from the heightmap
                int mapoffset = ((MAP_N * ((int)(ry) & (MAP_N - 1))) + ((int)(rx) & (MAP_N - 1)));

                // Project height values and find the height on-screen
                int projheight = (int)((camera.height - heightmap[ mapoffset ]) / z * SCALE_FACTOR + camera.horizon);

                // Only draw pixels if the new projected height is taller than the previous tallest height
                if (projheight < tallestheight) {
                    // Draw pixels from previous max-height until the new projected height
                    for (int y = projheight; y < tallestheight; y++) {
                        if (y >= 0) {
                            Draw( i, y, colormap->GetPixel( rx, ry ));
                        }
                    }
                    tallestheight = projheight;
                }
            }
        }
        // display camera position and orientation info and map number
        int nDegrees = (int( camera.angle * 180.0f / 3.141592f ) + 360) % 360;
        DrawString( 10, 10, "Camera: x = " + std::to_string( camera.x      ), olc::YELLOW );
        DrawString( 10, 20, "        y = " + std::to_string( camera.y      ), olc::YELLOW );
        DrawString( 10, 30, "        h = " + std::to_string( camera.height ), olc::YELLOW );
        DrawString( 10, 40, "        a = " + std::to_string( nDegrees      ), olc::YELLOW );
        DrawString( 10, 60, "Map index = " + std::to_string( nActiveMap    ), olc::YELLOW );

        return true;
    }
};


int main()
{
	VoxelSpace demo;
	if (demo.Construct( SCREEN_X / PIXEL_X, SCREEN_Y / PIXEL_Y, PIXEL_X, PIXEL_Y )) {
		demo.Start();
	}

	return 0;
}

