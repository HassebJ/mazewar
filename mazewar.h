/* $Header: mazewar.h,v 1.7 88/08/25 09:59:51 kent Exp $ */

/*
 * mazewar.h - Definitions for MazeWar
 *
 * Author:	Christopher A. Kent
 * 		Western Research Laboratory
 * 		Digital Equipment Corporation
 * Date:	Wed Sep 24 1986
 */

/* Modified by Michael Greenwald for CS244B, Mar 1992,
   Greenwald@cs.stanford.edu */

/* Modified by Nicholas Dovidio for CS244B, Mar 2009,
 * ndovidio@stanford.edu
 * This version now uses the CS249a/b style of C++ coding.
 */

/***********************************************************
Copyright 1986 by Digital Equipment Corporation, Maynard, Massachusetts,

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Digital not be
used in advertising or publicity pertaining to disstribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

#ifndef MAZEWAR_H
#define MAZEWAR_H


#include "fwk/NamedInterface.h"

#include "Nominal.h"
#include "Exception.h"
#include <string>
#include <map>
#include <stdlib.h>
#include <time.h>
/* fundamental constants */

#ifndef	TRUE
#define	TRUE		1
#define	FALSE		0
#endif	/* TRUE */

/* You can modify this if you want to */
#define	MAX_RATS	8

/* network stuff */
/* Feel free to modify.  This is the simplest version we came up with */

/* A unique MAZEPORT will be assigned to your team by the TA */
#define	MAZEPORT	5000
/* The multicast group for Mazewar is 224.1.1.1 */
#define MAZEGROUP       0xe0010101
#define	MAZESERVICE	"mazewar244B"

/* The next two >must< be a power of two, because we subtract 1 from them
   to get a bitmask for random()
 */
#define	MAZEXMAX	32
#define	MAZEYMAX	16
#define	VECTORSIZE	55
#define	NAMESIZE	20
#define	NDIRECTION	4
#define	NORTH		0
#define	SOUTH		1
#define	EAST		2
#define	WEST		3
#define	NVIEW		4
#define	LEFT		0
#define	RIGHT		1
#define	REAR		2
#define	FRONT		3
#define NEW          0
#define MOV          1
#define FIR          2
#define HIT          3
#define EXT          4
#define BEAT          5


/* types */
//   using namespace std;


typedef	struct sockaddr_in			Sockaddr;
typedef bool	               		MazeRow[MAZEYMAX];
typedef	MazeRow						MazeType [MAZEXMAX];
typedef	MazeRow						*MazeTypePtr;
//typedef	short						Direction;
typedef	struct {short	x, y; }		XYpoint;
typedef	struct {XYpoint	p1, p2;}	XYpair;
typedef	struct {short	xcor, ycor;}XY;
typedef	struct {unsigned short	bits[16];}	BitCell;
typedef	char						RatName[NAMESIZE];


template<class T>
struct map_data_compare : public std::binary_function<typename T::value_type,
typename T::mapped_type,
bool>
{
public:
    bool operator() (typename T::value_type &pair,
                     typename T::mapped_type i) const
    {
        return pair.second == i;
    }
};

typedef map<unsigned short, int> mapType;


 	class Direction : public Ordinal<Direction, short> {
	public:
		Direction(short num) : Ordinal<Direction, short>(num) {
			if(num<NORTH || num>NDIRECTION){
				throw RangeException("Error: Unexpected value.\n");
			}
		}
	};

 	class Loc : public Ordinal<Loc, short> {
	public:
		Loc(short num) : Ordinal<Loc, short>(num) {
			if(num<0){
				throw RangeException("Error: Unexpected negative value.\n");
			}
		}
	};

 	class Score : public Ordinal<Score, int> {
	public:
		Score(int num) : Ordinal<Score, int>(num) {}
	};


 	class RatIndexType : public Ordinal<RatIndexType, int> {
	public:
		RatIndexType(int num) : Ordinal<RatIndexType, int>(num) {
			if(num<0){
				throw RangeException("Error: Unexpected negative value.\n");
			}
		}
	};

 	class RatId : public Ordinal<RatId, unsigned short> {
	public:
		RatId(unsigned short num) : Ordinal<RatId, unsigned short>(num) {
		}
        RatId() : Ordinal<RatId, unsigned short>(-(short)rand()) {
		}
	};

 	class TokenId : public Ordinal<TokenId, long> {
	public:
		TokenId(long num) : Ordinal<TokenId, long>(num) {}
	};


class RatAppearance{

	public:
		RatAppearance() :  x(1), y(1), tokenId(0) {};
		bool	visible;
		Loc	x, y;
		short	distance;
		TokenId	tokenId;
};

class Rat{

public:
	Rat() :  playing(0), x(1), y(1), dir(NORTH), score(0){};
    Rat(RatId id_, Loc x_, Loc y_, Direction dir_, Score score_, RatName name_) : id(id_.value()), playing(1), x(x_.value()), y(y_.value()), dir(dir_.value()), score(score_.value()){
        strncpy(name, name_, NAMESIZE);
    }
    bool playing;
    RatId id;
	Loc	x, y;
	Direction dir;
    Score score;
    RatName name;
};



typedef	RatAppearance			RatApp_type [MAX_RATS];
typedef	RatAppearance *			RatLook;

/* defined in display.c */
extern RatApp_type 			Rats2Display;

/* variables "exported" by the mazewar "module" */
class MazewarInstance :  public Fwk::NamedInterface  {
 public:
    typedef Fwk::Ptr<MazewarInstance const> PtrConst;
    typedef Fwk::Ptr<MazewarInstance> Ptr;

	static MazewarInstance::Ptr mazewarInstanceNew(string s){
      MazewarInstance * m = new MazewarInstance(s);
      return m;
    }

    inline Direction dir() const { return dir_; }
    void dirIs(Direction dir) { this->dir_ = dir; }
    inline Direction dirPeek() const { return dirPeek_; }
    void dirPeekIs(Direction dirPeek) { this->dirPeek_ = dirPeek; }

    inline long mazePort() const { return mazePort_; }
    void mazePortIs(long  mazePort) { this->mazePort_ = mazePort; }
    inline Sockaddr* myAddr() const { return myAddr_; }
    void myAddrIs(Sockaddr *myAddr) { this->myAddr_ = myAddr; }
    inline RatId myRatId() const { return myRatId_; }
    void myRatIdIs(RatId myRatId) { this->myRatId_ = myRatId; }

    inline bool peeking() const { return peeking_; }
    void peekingIs(bool peeking) { this->peeking_ = peeking; }
    inline int theSocket() const { return theSocket_; }
    void theSocketIs(int theSocket) { this->theSocket_ = theSocket; }
    inline Score score() const { return score_; }
    void scoreIs(Score score) { this->score_ = score; }
    inline Loc xloc() const { return xloc_; }
    void xlocIs(Loc xloc) { this->xloc_ = xloc; }
    inline Loc yloc() const { return yloc_; }
    void ylocIs(Loc yloc) { this->yloc_ = yloc; }
    inline Loc xPeek() const { return xPeek_; }
    void xPeekIs(Loc xPeek) { this->xPeek_ = xPeek; }
    inline Loc yPeek() const { return yPeek_; }
    void yPeekIs(Loc yPeek) { this->yPeek_ = yPeek; }
    inline int active() const { return active_; }
    void activeIs(int active) { this->active_ = active; }
    inline Rat rat(RatIndexType num) const { return mazeRats_[num.value()]; }
    void ratIs(Rat rat, RatIndexType num) { this->mazeRats_[num.value()] = rat; }
    inline int getFreeIndex(){
        if (freeIndex >7)
            throw RangeException("Error: Number of Rats greater than MAX_RATS.\n");
        else
            return freeIndex;
    }

    MazeType maze_;
    bool isSet;
    RatName myName_;
    Rat mazeRats_[MAX_RATS];
//    map <unsigned short, int> indexToId;
    mapType indexToId;
    int freeIndex;
protected:
	MazewarInstance(string s) : Fwk::NamedInterface(s), dir_(0), dirPeek_(0), myRatId_(rand()), score_(0),
		xloc_(1), yloc_(3), xPeek_(0), yPeek_(0), isSet(false) {
            freeIndex = 0;
            myAddr_ = (Sockaddr*)malloc(sizeof(Sockaddr));
            if(!myAddr_) {
                printf("Error allocating sockaddr variable");
		}
	}
	Direction	dir_;
    Direction dirPeek_;

    long mazePort_;
    Sockaddr *myAddr_;
    
    

    
    RatId myRatId_;

    bool peeking_;
    int theSocket_;
    Score score_;
    Loc xloc_;
    Loc yloc_;
    Loc xPeek_;
    Loc yPeek_;
    int active_;
};
extern MazewarInstance::Ptr M;




#define MY_RAT_INDEX		0
#define MY_DIR			M->dir().value()
#define MY_X_LOC		M->xloc().value()
#define MY_Y_LOC		M->yloc().value()

/* events */

#define	EVENT_A		1		/* user pressed "A" */
#define	EVENT_S		2 		/* user pressed "S" */
#define	EVENT_D		3		/* user pressed "F" */
#define	EVENT_F		4		/* user pressed "D" */
#define	EVENT_BAR	5		/* user pressed space bar */
#define	EVENT_LEFT_D	6		/* user pressed left mouse button */
#define	EVENT_RIGHT_D	7		/* user pressed right button */
#define	EVENT_MIDDLE_D	8		/* user pressed middle button */
#define	EVENT_LEFT_U	9		/* user released l.M.b */
#define	EVENT_RIGHT_U	10		/* user released r.M.b */

#define	EVENT_NETWORK	16		/* incoming network packet */
#define	EVENT_INT	17		/* user pressed interrupt key */
#define	EVENT_TIMEOUT	18		/* nothing happened! */

extern unsigned short	ratBits[];
/* replace this with appropriate definition of your own */
typedef	struct MW244BPacket{
	unsigned char type;
	u_long	body[256];
    RatId ratId;
    
    MW244BPacket() : ratId(rand()){}
};

typedef	struct {
	RatId id;
	Loc	x;
    Loc y;
    Direction dir;
    RatName name;
}					NewPacket;

typedef	struct {
	RatId id;
	Loc	x;
    Loc y;
    Direction dir;
}					MovPacket;

typedef	struct{
//	bool playing, visible;
	Loc	x, y;
	Direction dir;
//	static int inflight;
    RatId id;
} FirPacket;


typedef	struct HitPacket{
	RatId preyId;
	Score preyScore;
    RatId predatorId;
	Score predatorScore;
    HitPacket ():predatorScore(0), preyScore(0){}
}					;

typedef	struct {
	Rat one;
    Rat two;
    Rat three;

    Rat four;
    Rat five;
    Rat six;
    Rat seven;
    Rat eight;
    
}					BeatPacket;

//typedef	struct {
//	RatId one;
//    RatId two;
//    RatId three;
//    RatId four;
//    RatId five;
//    RatId six;
//    RatId seven;
//    RatId eight;
//    
//}					BeatPacket;

typedef	struct {
	short		eventType;
	MW244BPacket	*eventDetail;	/* for incoming data */
	Sockaddr	eventSource;
}					MWEvent;

void		*malloc();
Sockaddr	*resolveHost();

/* display.c */
void InitDisplay(int, char **);
void StartDisplay(void);
void ShowView(Loc, Loc, Direction);
void SetMyRatIndexType(RatIndexType);
void SetRatPosition(RatIndexType, Loc, Loc, Direction);
void ClearRatPosition(RatIndexType);
void ShowPosition(Loc, Loc, Direction);
void ShowAllPositions(void);
void showMe(Loc, Loc, Direction);
void clearPosition(RatIndexType, Loc, Loc);
void clearSquare(Loc xClear, Loc yClear);
void NewScoreCard(void);
void UpdateScoreCard(RatIndexType);
void FlipBitmaps(void);
void bitFlip(BitCell *, int size);
void SwapBitmaps(void);
void byteSwap(BitCell *, int size);
void showMissile(Loc x_loc, Loc y_loc, Direction dir, Loc prev_x, Loc prev_y, bool clear);


/* init.c */
void MazeInit(int, char **);
void ratStates(void);
void getMaze(void);
void setRandom(void);
void getName(char *, char **);
void getString(char *, char **);
void getHostName(char *, char **, Sockaddr *);
Sockaddr *resolveHost(char *);
bool emptyAhead();
bool emptyRight();
bool emptyLeft();
bool emptyBehind();

/* toplevel.c */
void play(void);
void sendPacketToPlayer(HitPacket h , int packetType);
void createRatList(BeatPacket* list);
int idToIndex(const unsigned short value);
void aboutFace(void);
void leftTurn(void);
void rightTurn(void);
void forward(void);
void backward(void);
void peekLeft(void);
void peekRight(void);
void peekStop(void);
void shoot(void);
void quit(int);
void NewPosition(MazewarInstance::Ptr M);
void MWError(char *);
Score GetRatScore(RatIndexType);
char  *GetRatName(RatIndexType);
void ConvertIncoming(MW244BPacket *);
void ConvertOutgoing(MW244BPacket *);
void ratState(void);
void manageMissiles(void);
void DoViewUpdate(void);
void sendPacketToPlayer(RatId ratId , int packetType);
void processPacket(MWEvent *);
void netInit(void);



/* winsys.c */
void InitWindow(int, char **);
void StartWindow(int, int);
void ClearView(void);
void DrawViewLine(int, int, int, int);
void NextEvent(MWEvent *, int);
bool KBEventPending(void);
void HourGlassCursor(void);
void RatCursor(void);
void DeadRatCursor(void);
void HackMazeBitmap(Loc, Loc, BitCell *);
void DisplayRatBitmap(int, int, int, int, int, int);
void WriteScoreString(RatIndexType);
void ClearScoreLine(RatIndexType);
void InvertScoreLine(RatIndexType);
void NotifyPlayer(void);
void DrawString(const char*, uint32_t, uint32_t, uint32_t);
void StopWindow(void);

class Missile{
public:
	bool playing, visible;
	Loc	x, y;
	Direction dir;
	static int inflight;
    RatId id;
    
    
	Missile() :  playing(0), visible(1), x(1), y(1), dir(NORTH), id(M->myRatId()){
//        inflight++;
    };
    
    ~Missile(){
        this->inflight--;
        clearSquare(x, y);
    }
    
	Direction getdir() const
	{
	 	return dir;
	}
    void dirIs(Direction dir) {
    	this->dir = dir;
    }
    
	Loc xloc() const
	{
		return this->x;
	}
    void xlocIs(Loc xloc) {
    	this->x = xloc;
    }
    Loc yloc() const {
    	return y;
    }
    void ylocIs(Loc yloc) {
        this->y = yloc;
 	}
    
	void updateLoc(/*MazewarInstance * M*/){
		if(isVisible() == true){
			
			if (dir == NORTH){
				this->x = Loc(this->x.value() + 1);
			}
			else if(dir == SOUTH){
				this->x = Loc(this->x.value() - 1);
			}
			else if(dir == EAST){
				this->y = Loc(this->y.value() + 1);
			}
			else if(dir == WEST){
				this->y = Loc(this->y.value() - 1);
			}
            
		}
		else{
			this->visible = false;
            
		}
		
	}
    
	bool isVisible()
	{
		/* prevent a blank wall at first glimpse */
        
		register int	tx = this->x.value();
		register int	ty = this->y.value();
        
		if (tx >= MAZEXMAX || ty >= MAZEYMAX) {
            /* MAZE[XY]MAX is a power of 2 */
            return false;
            
            /* In real game, also check that square is
		     unoccupied by another rat */
		}
        
        
        
		switch(dir.value()) {
            case NORTH:
                if (M->maze_[tx+1][ty])
                    return false;
                break;
            case SOUTH:
                if (M->maze_[tx-1][ty])
                    return false;
                break;
            case EAST:	
                if (M->maze_[tx][ty+1])
                    return false; 
                break;
            case WEST:	
                if (M->maze_[tx][ty-1])	
                    return false; 
                break;
                // default:
                // MWError("bad direction in Forward");
		}
        
		return true;
		
	}
    
};



#endif
