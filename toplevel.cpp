
/*
 *   FILE: toplevel.c
 * AUTHOR: name (email)
 *   DATE: March 31 23:59:59 PST 2013
 *  DESCR:
 */

/* #define DEBUG */

#include "main.h"
#include <string>
#include "mazewar.h"

static bool		updateView;	/* true if update needed */
MazewarInstance::Ptr M;
int Missile::inflight = 0;

/* Use this socket address to send packets to the multi-cast group. */
static Sockaddr         groupAddr;
#define MAX_OTHER_RATS  (MAX_RATS - 1)

Missile * rockets[MAX_RATS];

int main(int argc, char *argv[])
{
    srand (time(NULL));

    Loc x(1);
    Loc y(5);
    Direction dir(0);
    char *ratName;

    signal(SIGHUP, quit);
    signal(SIGINT, quit);
    signal(SIGTERM, quit);

    getName("Welcome to CS244B MazeWar!\n\nYour Name", &ratName);
    ratName[strlen(ratName)-1] = 0;

    M = MazewarInstance::mazewarInstanceNew(string(ratName));
    MazewarInstance* a = M.ptr();
    strncpy(M->myName_, ratName, NAMESIZE);
    free(ratName);

    MazeInit(argc, argv);

    NewPosition(M);
    

    /* So you can see what a Rat is supposed to look like, we create
    one rat in the single player mode Mazewar.
    It doesn't move, you can't shoot it, you can just walk around it */

    play();

    return 0;
}


/* ----------------------------------------------------------------------- */

void
play(void)
{
	MWEvent		event;
	MW244BPacket	incoming;

	event.eventDetail = &incoming;

	while (TRUE) {
		NextEvent(&event, M->theSocket());
		if (!M->peeking())
			switch(event.eventType) {
			case EVENT_A:				
				leftTurn();
				break;

			case EVENT_S:
				aboutFace();
                        cout<<"ratId: "<<M->myRatId().value()<<endl;
				break;

			case EVENT_F:
				forward();
                sendPacketToPlayer(RatId(0), MOV);
				break;

			case EVENT_D:
				rightTurn();
				break;


			case EVENT_LEFT_D:
				peekLeft();
				break;

			case EVENT_BAR:
				shoot();
				break;

			case EVENT_RIGHT_D:
				peekRight();
				break;

			case EVENT_NETWORK:
				processPacket(&event);
				break;
                       
            case EVENT_TIMEOUT:
                manageMissiles();
                
                break;

			case EVENT_INT:
				quit(0);
				break;

			}
		else
			switch (event.eventType) {
			case EVENT_RIGHT_U:
			case EVENT_LEFT_U:
				peekStop();
				break;

			case EVENT_NETWORK:
				processPacket(&event);
				break;
			}

		ratStates();		/* clean house */

//		manageMissiles();

		DoViewUpdate();

		/* Any info to send over network? */

	}
}

/* ----------------------------------------------------------------------- */

static	Direction	_aboutFace[NDIRECTION] ={SOUTH, NORTH, WEST, EAST};
static	Direction	_leftTurn[NDIRECTION] =	{WEST, EAST, NORTH, SOUTH};
static	Direction	_rightTurn[NDIRECTION] ={EAST, WEST, SOUTH, NORTH};

void
aboutFace(void)
{
	M->dirIs(_aboutFace[MY_DIR]);
	updateView = TRUE;
}

/* ----------------------------------------------------------------------- */

void
leftTurn(void)
{
	M->dirIs(_leftTurn[MY_DIR]);
	updateView = TRUE;
}

/* ----------------------------------------------------------------------- */

void
rightTurn(void)
{
	M->dirIs(_rightTurn[MY_DIR]);
	updateView = TRUE;
}

/* ----------------------------------------------------------------------- */

/* remember ... "North" is to the right ... positive X motion */

void
forward(void)
{
	register int	tx = MY_X_LOC;
	register int	ty = MY_Y_LOC;

	switch(MY_DIR) {
	case NORTH:	if (!M->maze_[tx+1][ty])	tx++; break;
	case SOUTH:	if (!M->maze_[tx-1][ty])	tx--; break;
	case EAST:	if (!M->maze_[tx][ty+1])	ty++; break;
	case WEST:	if (!M->maze_[tx][ty-1])	ty--; break;
	default:
		MWError("bad direction in Forward");
	}
	if ((MY_X_LOC != tx) || (MY_Y_LOC != ty)) {
		M->xlocIs(Loc(tx));
		M->ylocIs(Loc(ty));
		updateView = TRUE;
	}
}

/* ----------------------------------------------------------------------- */

void backward()
{
	register int	tx = MY_X_LOC;
	register int	ty = MY_Y_LOC;

	switch(MY_DIR) {
	case NORTH:	if (!M->maze_[tx-1][ty])	tx--; break;
	case SOUTH:	if (!M->maze_[tx+1][ty])	tx++; break;
	case EAST:	if (!M->maze_[tx][ty-1])	ty--; break;
	case WEST:	if (!M->maze_[tx][ty+1])	ty++; break;
	default:
		MWError("bad direction in Backward");
	}
	if ((MY_X_LOC != tx) || (MY_Y_LOC != ty)) {
		M->xlocIs(Loc(tx));
		M->ylocIs(Loc(ty));
		updateView = TRUE;
	}
}

/* ----------------------------------------------------------------------- */

void peekLeft()
{
	M->xPeekIs(MY_X_LOC);
	M->yPeekIs(MY_Y_LOC);
	M->dirPeekIs(MY_DIR);

	switch(MY_DIR) {
	case NORTH:	if (!M->maze_[MY_X_LOC+1][MY_Y_LOC]) {
				M->xPeekIs(MY_X_LOC + 1);
				M->dirPeekIs(WEST);
			}
			break;

	case SOUTH:	if (!M->maze_[MY_X_LOC-1][MY_Y_LOC]) {
				M->xPeekIs(MY_X_LOC - 1);
				M->dirPeekIs(EAST);
			}
			break;

	case EAST:	if (!M->maze_[MY_X_LOC][MY_Y_LOC+1]) {
				M->yPeekIs(MY_Y_LOC + 1);
				M->dirPeekIs(NORTH);
			}
			break;

	case WEST:	if (!M->maze_[MY_X_LOC][MY_Y_LOC-1]) {
				M->yPeekIs(MY_Y_LOC - 1);
				M->dirPeekIs(SOUTH);
			}
			break;

	default:
			MWError("bad direction in PeekLeft");
	}

	/* if any change, display the new view without moving! */

	if ((M->xPeek() != MY_X_LOC) || (M->yPeek() != MY_Y_LOC)) {
		M->peekingIs(TRUE);
		updateView = TRUE;
	}
}

/* ----------------------------------------------------------------------- */

void peekRight()
{
	M->xPeekIs(MY_X_LOC);
	M->yPeekIs(MY_Y_LOC);
	M->dirPeekIs(MY_DIR);

	switch(MY_DIR) {
	case NORTH:	if (!M->maze_[MY_X_LOC+1][MY_Y_LOC]) {
				M->xPeekIs(MY_X_LOC + 1);
				M->dirPeekIs(EAST);
			}
			break;

	case SOUTH:	if (!M->maze_[MY_X_LOC-1][MY_Y_LOC]) {
				M->xPeekIs(MY_X_LOC - 1);
				M->dirPeekIs(WEST);
			}
			break;

	case EAST:	if (!M->maze_[MY_X_LOC][MY_Y_LOC+1]) {
				M->yPeekIs(MY_Y_LOC + 1);
				M->dirPeekIs(SOUTH);
			}
			break;

	case WEST:	if (!M->maze_[MY_X_LOC][MY_Y_LOC-1]) {
				M->yPeekIs(MY_Y_LOC - 1);
				M->dirPeekIs(NORTH);
			}
			break;

	default:
			MWError("bad direction in PeekRight");
	}

	/* if any change, display the new view without moving! */

	if ((M->xPeek() != MY_X_LOC) || (M->yPeek() != MY_Y_LOC)) {
		M->peekingIs(TRUE);
		updateView = TRUE;
	}
}

/* ----------------------------------------------------------------------- */

void peekStop()
{
	M->peekingIs(FALSE);
	updateView = TRUE;
}

/* ----------------------------------------------------------------------- */
void shoot()
{
	M->scoreIs( M->score().value()-1 );
    M->mazeRats_[MY_RAT_INDEX].score = Score(M->mazeRats_[MY_RAT_INDEX].score.value() - 1);
	UpdateScoreCard( MY_RAT_INDEX);
	rockets[Missile::inflight] = new Missile();
	rockets[Missile::inflight]->xlocIs(Loc(MY_X_LOC));
	rockets[Missile::inflight]->ylocIs(Loc(MY_Y_LOC));
	rockets[Missile::inflight]->dirIs(Direction(MY_DIR));
	cout << rockets[Missile::inflight]->x.value() << " " << rockets[Missile::inflight]->y.value() << " "<< rockets[Missile::inflight]->x.value() <<" "<< rockets[Missile::inflight]->dir.value()<<endl;

	rockets[Missile::inflight]->updateLoc();

	cout << rockets[Missile::inflight]->x.value() << " " << rockets[Missile::inflight]->y.value() << " "<< rockets[Missile::inflight]->x.value() <<" "<< rockets[Missile::inflight]->dir.value()<<endl;
	if(rockets[Missile::inflight]->visible == true){
		showMissile(rockets[Missile::inflight]->xloc(), rockets[Missile::inflight]->yloc(), rockets[Missile::inflight]->getdir(), MY_X_LOC, MY_X_LOC, FALSE);
     
        sendPacketToPlayer(RatId(0), FIR);
        
        Missile::inflight++;
        
	}
    else{
        delete rockets[Missile::inflight];
    }
    
    updateView = TRUE;
	
	// cout << MY_X_LOC << MY_Y_LOC << endl;
}

/* ----------------------------------------------------------------------- */

/*
 * Exit from game, clean up window
 */

void quit(int sig)
{

	StopWindow();
	exit(0);
}


/* ----------------------------------------------------------------------- */




void NewPosition(MazewarInstance::Ptr m)
{
	Loc newX(0);
	Loc newY(0);
	Direction dir(0); /* start on occupied square */

	while (M->maze_[newX.value()][newY.value()]) {
	  /* MAZE[XY]MAX is a power of 2 */
	  newX = Loc(random() & (MAZEXMAX - 1));
	  newY = Loc(random() & (MAZEYMAX - 1));

	  /* In real game, also check that square is
	     unoccupied by another rat */
	}

	/* prevent a blank wall at first glimpse */

	if (!m->maze_[(newX.value())+1][(newY.value())]) dir = Direction(NORTH);
	if (!m->maze_[(newX.value())-1][(newY.value())]) dir = Direction(SOUTH);
	if (!m->maze_[(newX.value())][(newY.value())+1]) dir = Direction(EAST);
	if (!m->maze_[(newX.value())][(newY.value())-1]) dir = Direction(WEST);

	m->xlocIs(newX);
	m->ylocIs(newY);
	m->dirIs(dir);
}

/* ----------------------------------------------------------------------- */



/* ----------------------------------------------------------------------- */
void MWError(char *s)

{
	StopWindow();
	fprintf(stderr, "CS244BMazeWar: %s\n", s);
	perror("CS244BMazeWar");
	exit(-1);
}

/* ----------------------------------------------------------------------- */

/* This is just for the sample version, rewrite your own */
Score GetRatScore(RatIndexType ratId)
{
    unsigned short id = M->indexToId[ratId.value()];
    for (int i = 0; i<MAX_RATS;i++){
        if (M->mazeRats_[i].id.value() == id) {
            return M->mazeRats_[i].score;
        }
    }
//    cout << ratId.value() <<" "<< 	M->myRatId().value()<<endl;
//  if (ratId.value() == 	M->myRatId().value())
//    { return(M->score()); }
//  else { return (0); }
//    return (M->score());
    return (0);
    
}

/* ----------------------------------------------------------------------- */

/* This is just for the sample version, rewrite your own */
char *GetRatName(RatIndexType ratId)
{
    unsigned short id = M->indexToId[ratId.value()];
    for (int i = 0; i<MAX_RATS;i++){
        if (M->mazeRats_[i].id.value() == id) {
            return M->mazeRats_[i].name;
        }
    }
//  if (ratId.value() ==	M->myRatId().value())
//    { return(M->myName_); }
//  else { return ("Dummy"); }
//    return(M->myName_);
    return ("Dummy");
}

/* ----------------------------------------------------------------------- */

/* This is just for the sample version, rewrite your own if necessary */
void ConvertIncoming(MW244BPacket *p)
{
}

/* ----------------------------------------------------------------------- */

/* This is just for the sample version, rewrite your own if necessary */
void ConvertOutgoing(MW244BPacket *p)
{
}

/* ----------------------------------------------------------------------- */

/* This is just for the sample version, rewrite your own */
void ratStates()
{
  /* In our sample version, we don't know about the state of any rats over
     the net, so this is a no-op */
}

/* ----------------------------------------------------------------------- */

/* This is just for the sample version, rewrite your own */
void manageMissiles()
{
	// for(int i=0; i<MAX_RATS; i++){
    if (Missile::inflight - 1 >= MAX_RATS) {
        for(int i = 0; i < Missile::inflight -1; i++){
            delete rockets[i];
        }
        return;
    }

	
	

	for(int i = 0; i < Missile::inflight ;i++ ){
		Loc ox = rockets[i]->xloc();
		Loc oy = rockets[i]->yloc();
		rockets[i]->updateLoc();
		if(rockets[i]->visible == true){
		
			showMissile(rockets[i]->xloc(), rockets[i]->yloc(), rockets[i]->getdir(), ox, oy, TRUE);
            for(int j = 0; j < MAX_RATS; j++){
                if(rockets[i]->xloc().value() == M->mazeRats_[j].x.value() && rockets[i]->yloc().value() == M->mazeRats_[j].y.value()){
                    if(M->mazeRats_[j].id.value() == M->myRatId().value()){
                        M->scoreIs( M->score().value()-5 );
                    }
                    
                    HitPacket h;
                    
                    int index = idToIndex(M->mazeRats_[j].id.value());
                    h.preyId = M->mazeRats_[j].id;
                    M->mazeRats_[index].score = Score(M->mazeRats_[index].score.value() - 5);
                    h.preyScore = M->mazeRats_[index].score;
                    UpdateScoreCard(RatIndexType(index));
                    
                    index = idToIndex(rockets[i]->id.value());
                    h.predatorId = rockets[i]->id;
                    M->mazeRats_[index].score = Score(M->mazeRats_[index].score.value() + 10);
                    h.predatorScore = M->mazeRats_[index].score;
                    UpdateScoreCard(RatIndexType(index));
                    
                    sendPacketToPlayer(h , HIT);
                    
                    rockets[i]->visible = false;
                    delete rockets[i];
                    
                    
                    
                    
                }
            }

			

			
		}
		else{
			delete rockets[i];
		}

	}
    updateView = TRUE;

  
}

/* ----------------------------------------------------------------------- */

void DoViewUpdate()
{
	if (updateView) {	/* paint the screen */
		ShowPosition(MY_X_LOC, MY_Y_LOC, MY_DIR);
		if (M->peeking())
			ShowView(M->xPeek(), M->yPeek(), M->dirPeek());
		else
			ShowView(MY_X_LOC, MY_Y_LOC, MY_DIR);
		updateView = FALSE;
	}
}

/* ----------------------------------------------------------------------- */

/*
 * Sample code to send a packet to a specific destination
 */

/*
 * Notice the call to ConvertOutgoing.  You might want to call ConvertOutgoing
 * before any call to sendto.
 */

void sendPacketToPlayer(RatId ratId , int packetType)
{
    MW244BPacket pack;
    pack.type = packetType;
    pack.ratId = M->myRatId();
    NewPacket *_new;
    BeatPacket *beat;
    MovPacket * mov ;
    FirPacket * fir;
    
    printf("%d sent\n", pack.type);

    
    switch(packetType) {
        case NEW:
        {
            _new = (NewPacket *) &(pack.body);
            _new->id = M->myRatId();
            _new->x = M->xloc();
            _new->y = M->yloc();
            _new->dir = M->dir();
            strncpy(_new->name, M->myName_, NAMESIZE);
            
        
            break;
        }
        case BEAT:
        {
            cout << MY_X_LOC<<":"<< MY_Y_LOC<<endl;
            beat = (BeatPacket *) &(pack.body);
            createRatList(beat);
            
            break;
        }
        case MOV:
        {
            mov = (MovPacket *) &(pack.body);
            mov->id = M->myRatId();
            mov->x = M->xloc();
            mov->y = M->yloc();
            mov->dir = M->dir();
            
            break;
        }
        case FIR:
        {
            fir = (FirPacket *) &(pack.body);
            fir->id = M->myRatId();
            fir->x = rockets[Missile::inflight]->x;
            fir->y = rockets[Missile::inflight]->y;
            fir->dir = rockets[Missile::inflight]->dir;
            
            break;
        }
        case HIT:{
        
        
            break;
        }
        case EXT:
        {
            break;
        }
    }

    
    //.... set other fields in the packet  that you need to set...
    
    ConvertOutgoing(&pack);
    
    if (sendto((int)M->theSocket(), &pack, sizeof(pack), 0,
        (const sockaddr*)&groupAddr, sizeof(Sockaddr)) < 0)
    { MWError("Sample error") ;}
}

void sendPacketToPlayer(HitPacket h , int packetType)
{
    MW244BPacket pack;
    pack.type = packetType;
    pack.ratId = M->myRatId();
    HitPacket * hit ;
    
    printf("%d sent\n", pack.type);
    
    
    switch(packetType) {
        case HIT :{
            hit = (HitPacket *) &(pack.body);
            hit->preyId = h.preyId;
            hit->preyScore = h.preyScore;
            hit->predatorId = h.predatorId;
            hit->predatorScore = h.predatorScore;

            
            break;
        }
        case EXT:
        {
            break;
        }
    }
    
    
    //.... set other fields in the packet  that you need to set...
    
    ConvertOutgoing(&pack);
    
    if (sendto((int)M->theSocket(), &pack, sizeof(pack), 0,
               (const sockaddr*)&groupAddr, sizeof(Sockaddr)) < 0)
    { MWError("Sample error") ;}
}

/* ----------------------------------------------------------------------- */

int idToIndex(const unsigned short value){
    
    
    map<unsigned short, int>::iterator it  = find_if( M->indexToId.begin(), M->indexToId.end(), bind2nd(map_data_compare<mapType>(), value) );
    
    
    if ( it != M->indexToId.end() )
    {
        
//        std::cout << "Found index:" << it->first << " for value:" << it->second << std::endl;
        return it->first;
    }
//    else
//    {
//        std::cout << "Error: move packet receicved for unknown RatId" << value << std::endl;
//    }
}

/* Sample of processPacket. */

void processPacket (MWEvent *eventPacket)
{
    
    MW244BPacket            *pack = eventPacket->eventDetail;
       if (M->myRatId() == pack->ratId){
//           cout <<"returning"<<endl;
        return;
    }
    printf("%d received\n", pack->type);
//    cout <<M->myRatId().value()<<" : "<<pack->ratId.value()<<endl;
   
    NewPacket                *_new;
     
     switch(pack->type) {
         case NEW:
         {
             _new = (NewPacket *) &(pack->body);
             if (_new->id.value() < 8) {
//                 SetRatPosition(RatIndexType (M->getFreeIndex()), _new->x, _new->y, _new->dir);
                 
                 M->ratIs(Rat(_new->id, _new->x, _new->y, _new->dir, Score(0), _new->name), RatIndexType(M->getFreeIndex()));
                 M->indexToId[M->getFreeIndex()] = _new->id.value();
                 
                 
                 SetRatPosition(RatIndexType (M->getFreeIndex()), _new->x, _new->y, _new->dir);
                 UpdateScoreCard(RatIndexType(M->getFreeIndex()));
                 
                                 M->freeIndex++;
                 
                 
             }
             else{
                 
                 sendPacketToPlayer(RatId(0), BEAT);
             }
             

             
             break;
         }
         case MOV:
         {
             MovPacket * mov = (MovPacket *) &(pack->body);
             
             int index = idToIndex(mov->id.value());
             SetRatPosition(RatIndexType(index), mov->x, mov->y, mov->dir);
             
//             const unsigned short value = mov->id.value();
//             
//             map<unsigned short, int>::iterator it  = find_if( M->indexToId.begin(), M->indexToId.end(), bind2nd(map_data_compare<mapType>(), value) );
//             
//             
//             if ( it != M->indexToId.end() )
//             {
//               
//                 std::cout << "Found index:" << it->first << " for value:" << it->second << std::endl;
//                 SetRatPosition(RatIndexType(it->first), mov->x, mov->y, mov->dir);
//             }
//             else
//             {
//                 std::cout << "Error: move packet receicved for unknown RatId" << value << std::endl;
//             }
             
             
             break;
         }
         case FIR:
         {
             FirPacket *fir = (FirPacket *) &(pack->body);
             rockets[Missile::inflight] = new Missile();
             rockets[Missile::inflight]->xlocIs(fir->x);
             rockets[Missile::inflight]->ylocIs(fir->y);
             rockets[Missile::inflight]->dirIs(fir->dir);
             rockets[Missile::inflight]->dirIs(fir->dir);
             rockets[Missile::inflight]->playing = true;
             
             break;
         }
         case HIT:
         {
             HitPacket * hit = (HitPacket *) &(pack->body);
             
             int index = idToIndex(hit->preyId.value());
             M->mazeRats_[index].score = hit->preyScore;
             UpdateScoreCard(RatIndexType(index));
             
             index = idToIndex(hit->predatorId.value());
             M->mazeRats_[index].score = hit->predatorScore;
             UpdateScoreCard(RatIndexType(index));


             break;
         }
         case EXT:
         {break;}
         case BEAT:
         {
             BeatPacket *beat  = (BeatPacket *) &(pack->body);
             
             unsigned short temp = beat->one.id.value();
             if(beat->one.id.value() < 8)
                 temp = beat->one.id.value();
             if(beat->two.id.value() > temp && beat->two.id.value() < 8)
                 temp = beat->two.id.value();
             if(beat->three.id.value() > temp && beat->three.id.value() < 8)
                 temp = beat->three.id.value();
             if(beat->four.id.value() > temp && beat->four.id.value() < 8)
                 temp = beat->four.id.value();
             if(beat->five.id.value() > temp && beat->five.id.value() < 8)
                 temp = beat->five.id.value();
             if(beat->six.id.value() > temp && beat->six.id.value() < 8)
                 temp = beat->six.id.value();
             if(beat->seven.id.value() > temp && beat->seven.id.value() < 8)
                 temp = beat->seven.id.value();
             
             M->myRatIdIs(RatId(temp+1));
             M->scoreIs(0);
             SetMyRatIndexType(0);
             M->freeIndex++;
             
             M->ratIs(Rat(M->myRatId(), M->xloc(), M->yloc(), M->dir(), M->score(), M->myName_), RatIndexType(0));
             M->indexToId[0] = temp + 1;
             M->isSet = true;
             
             if(beat->one.id.value() < 8){
                 M->ratIs(Rat(beat->one.id, beat->one.x, beat->one.y, beat->one.dir, beat->one.score, beat->one.name), RatIndexType(M->getFreeIndex()));
                 M->indexToId[M->getFreeIndex()] = beat->one.id.value();
                 
                 SetRatPosition(RatIndexType (M->getFreeIndex()), beat->one.x, beat->one.y, beat->one.dir);
                 M->freeIndex++;
               
                 
                 
             }
             if(beat->two.id.value() < 8){
                 M->ratIs(Rat(beat->two.id, beat->two.x, beat->two.y, beat->two.dir, beat->two.score, beat->two.name), RatIndexType(M->getFreeIndex()));
                 M->indexToId[M->getFreeIndex()] = beat->two.id.value();
                 SetRatPosition(RatIndexType (M->getFreeIndex()), beat->two.x, beat->two.y, beat->two.dir);
                 M->freeIndex++;
             }
             if(beat->three.id.value() < 8){
                 M->ratIs(Rat(beat->three.id, beat->three.x, beat->three.y, beat->three.dir, beat->three.score, beat->three.name), RatIndexType(M->getFreeIndex()));
                 M->indexToId[M->getFreeIndex()] = beat->three.id.value();
                                  SetRatPosition(RatIndexType (M->getFreeIndex()), beat->three.x, beat->three.y, beat->three.dir);
                 M->freeIndex++;
             }
             if(beat->four.id.value() < 8){
                 M->ratIs(Rat(beat->four.id, beat->four.x, beat->four.y, beat->four.dir, beat->four.score, beat->four.name), RatIndexType(M->getFreeIndex()));
                 M->indexToId[M->getFreeIndex()] = beat->four.id.value();
                                  SetRatPosition(RatIndexType (M->getFreeIndex()), beat->four.x, beat->four.y, beat->four.dir);
                 M->freeIndex++;
             }
             if(beat->five.id.value() < 8){
                 M->ratIs(Rat(beat->five.id, beat->five.x, beat->five.y, beat->five.dir, beat->five.score, beat->five.name), RatIndexType(M->getFreeIndex()));
                 M->indexToId[M->getFreeIndex()] = beat->five.id.value();
                                  SetRatPosition(RatIndexType (M->getFreeIndex()), beat->five.x, beat->five.y, beat->five.dir);
                 M->freeIndex++;
             }
             if(beat->six.id.value() < 8){
                 M->ratIs(Rat(beat->six.id, beat->six.x, beat->six.y, beat->six.dir, beat->six.score, beat->six.name), RatIndexType(M->getFreeIndex()));
                 M->indexToId[M->getFreeIndex()] = beat->six.id.value();
                                  SetRatPosition(RatIndexType (M->getFreeIndex()), beat->six.x, beat->six.y, beat->six.dir);
                 M->freeIndex++;
             }
             if(beat->seven.id.value() < 8){
                 M->ratIs(Rat(beat->seven.id, beat->seven.x, beat->seven.y, beat->seven.dir, beat->seven.score, beat->seven.name), RatIndexType(M->getFreeIndex()));
                 M->indexToId[M->getFreeIndex()] = beat->seven.id.value();
                                  SetRatPosition(RatIndexType (M->getFreeIndex()), beat->seven.x, beat->seven.y, beat->seven.dir);
                 M->freeIndex++;
             }
             if(beat->eight.id.value() < 8){
                 M->ratIs(Rat(beat->eight.id, beat->eight.x, beat->eight.y, beat->eight.dir, beat->eight.score, beat->eight.name), RatIndexType(M->getFreeIndex()));
                 M->indexToId[M->getFreeIndex()] = beat->eight.id.value();
                                  SetRatPosition(RatIndexType (M->getFreeIndex()), beat->eight.x, beat->eight.y, beat->eight.dir);
                 M->freeIndex++;
             }
             
             sendPacketToPlayer(M->myRatId(), NEW);
             
             
             
             
             break;
         }
     }
    
     
//    updateView = TRUE; 

}

/* ----------------------------------------------------------------------- */

/* This will presumably be modified by you.
   It is here to provide an example of how to open a UDP port.
   You might choose to use a different strategy
 */
//void createRatList (BeatPacket* list){
//    RatId temp;
//    
//    temp = M->mazeRats_[0].id;
//    list->one = temp;
//    
//    temp = M->mazeRats_[1].id;
//    list->two = temp;
//    
//    temp = M->mazeRats_[2].id;
//    list->three = temp;
//    
//    temp = M->mazeRats_[3].id;
//    list->four = temp;
//    
//    temp = M->mazeRats_[4].id;
//    list->five = temp;
//    
//    temp = M->mazeRats_[5].id;
//    list->six = temp;
//    
//    temp = M->mazeRats_[6].id;
//    list->seven = temp;
//    
//    temp = M->mazeRats_[7].id;
//    list->eight = temp;
//    
//}


void createRatList(BeatPacket* list){
    Rat temp;
    temp.id = M->mazeRats_[0].id;
    temp.x = M->mazeRats_[0].x;
    temp.y = M->mazeRats_[0].y;
    temp.dir = M->mazeRats_[0].dir;
    temp.score = M->mazeRats_[0].score;
    strncpy(temp.name, M->mazeRats_[0].name, NAMESIZE);

    
    list->one =temp;
    
    temp.id = M->mazeRats_[1].id;
    temp.x = M->mazeRats_[1].x;
    temp.y = M->mazeRats_[1].y;
    temp.dir = M->mazeRats_[1].dir;
    temp.score = M->mazeRats_[1].score;
    strncpy(temp.name, M->mazeRats_[1].name, NAMESIZE);
    
    list->two =temp;
    
    temp.id = M->mazeRats_[2].id;
    temp.x = M->mazeRats_[2].x;
    temp.y = M->mazeRats_[2].y;
    temp.dir = M->mazeRats_[2].dir;
    temp.score = M->mazeRats_[2].score;
    strncpy(temp.name, M->mazeRats_[2].name, NAMESIZE);
    
    list->three =temp;
    
    temp.id = M->mazeRats_[3].id;
    temp.x = M->mazeRats_[3].x;
    temp.y = M->mazeRats_[3].y;
    temp.dir = M->mazeRats_[3].dir;
    temp.score = M->mazeRats_[3].score;
    strncpy(temp.name, M->mazeRats_[3].name, NAMESIZE);
    
    list->four =temp;
    
    temp.id = M->mazeRats_[4].id;
    temp.x = M->mazeRats_[4].x;
    temp.y = M->mazeRats_[4].y;
    temp.dir = M->mazeRats_[4].dir;
    temp.score = M->mazeRats_[4].score;
    strncpy(temp.name, M->mazeRats_[4].name, NAMESIZE);
    
    list->five =temp;
    
    temp.id = M->mazeRats_[5].id;
    temp.x = M->mazeRats_[5].x;
    temp.y = M->mazeRats_[5].y;
    temp.dir = M->mazeRats_[5].dir;
    temp.score = M->mazeRats_[5].score;
    strncpy(temp.name, M->mazeRats_[5].name, NAMESIZE);
    
    list->six =temp;
    
    temp.id = M->mazeRats_[6].id;
    temp.x = M->mazeRats_[6].x;
    temp.y = M->mazeRats_[6].y;
    temp.dir = M->mazeRats_[6].dir;
    temp.score = M->mazeRats_[6].score;
    strncpy(temp.name, M->mazeRats_[6].name, NAMESIZE);
    
    list->seven =temp;
    
    temp.id = M->mazeRats_[7].id;
    temp.x = M->mazeRats_[7].x;
    temp.y = M->mazeRats_[7].y;
    temp.dir = M->mazeRats_[7].dir;
    temp.score = M->mazeRats_[7].score;
    strncpy(temp.name, M->mazeRats_[7].name, NAMESIZE);
    
    list->eight =temp;

    
}

void
netInit()
{
	Sockaddr		nullAddr;
	Sockaddr		*thisHost;
	char			buf[128];
	int				reuse;
	u_char          ttl;
	struct ip_mreq  mreq;

	/* MAZEPORT will be assigned by the TA to each team */
	M->mazePortIs(htons(MAZEPORT));

	gethostname(buf, sizeof(buf));
	if ((thisHost = resolveHost(buf)) == (Sockaddr *) NULL)
	  MWError("who am I?");
	bcopy((caddr_t) thisHost, (caddr_t) (M->myAddr()), sizeof(Sockaddr));

	M->theSocketIs(socket(AF_INET, SOCK_DGRAM, 0));
	if (M->theSocket() < 0)
	  MWError("can't get socket");

	/* SO_REUSEADDR allows more than one binding to the same
	   socket - you cannot have more than one player on one
	   machine without this */
	reuse = 1;
	if (setsockopt(M->theSocket(), SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0) {
		MWError("setsockopt failed (SO_REUSEADDR)");
	}

	nullAddr.sin_family = AF_INET;
	nullAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	nullAddr.sin_port = M->mazePort();
	if (bind(M->theSocket(), (struct sockaddr *)&nullAddr,
		 sizeof(nullAddr)) < 0)
	  MWError("netInit binding");

	/* Multicast TTL:
	   0 restricted to the same host
	   1 restricted to the same subnet
	   32 restricted to the same site
	   64 restricted to the same region
	   128 restricted to the same continent
	   255 unrestricted

	   DO NOT use a value > 32. If possible, use a value of 1 when
	   testing.
	*/

	ttl = 1;
	if (setsockopt(M->theSocket(), IPPROTO_IP, IP_MULTICAST_TTL, &ttl,
		   sizeof(ttl)) < 0) {
		MWError("setsockopt failed (IP_MULTICAST_TTL)");
	}

	/* join the multicast group */
	mreq.imr_multiaddr.s_addr = htonl(MAZEGROUP);
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	if (setsockopt(M->theSocket(), IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)
		   &mreq, sizeof(mreq)) < 0) {
		MWError("setsockopt failed (IP_ADD_MEMBERSHIP)");
	}

	/*
	 * Now we can try to find a game to join; if none, start one.
	 */
	 
	printf("\n");

	/* set up some stuff strictly for this local sample */
    
  
	

	/* Get the multi-cast address ready to use in SendData()
           calls. */
	memcpy(&groupAddr, &nullAddr, sizeof(Sockaddr));
	groupAddr.sin_addr.s_addr = htonl(MAZEGROUP);
    
    
    sendPacketToPlayer(RatId(rand()), NEW);
    
    
    
    struct timeval oldtime,newtime;
    
    gettimeofday(&oldtime,0);
    gettimeofday(&newtime,0);
    bool gotPacket = false;
    
    
    do{
        gettimeofday(&newtime,0);
        
        MWEvent		event;
        MW244BPacket	incoming;
        
        event.eventDetail = &incoming;
        NextEvent(&event, M->theSocket());
        
        switch(event.eventType) {
                
            case EVENT_NETWORK:
                processPacket(&event);
                gotPacket = true;
                break;
                
        }
        
        //cout << newtime.tv_sec  <<" : " << oldtime.tv_sec + 0.5 <<"\n";
        
        
    } while (newtime.tv_sec < oldtime.tv_sec + 1.5 );
    
    if (M->isSet == false) {
        M->myRatIdIs(0);
        M->scoreIs(0);
        SetMyRatIndexType(0);
        
        M->ratIs(Rat(M->myRatId(), M->xloc(), M->yloc(), M->dir(), Score(0), M->myName_), RatIndexType(0));
        M->indexToId[0] = 0;
        M->isSet = true;
        M->freeIndex++;
        
//        BeatPacket *beat;
//        
//        createRatList(beat);
//        
//        BeatPacket *t = beat;
        
        
        
    }
    else{
        //cout <<"NewPacket received"<<endl;
    }


}


/* ----------------------------------------------------------------------- */
