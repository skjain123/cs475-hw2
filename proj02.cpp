#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>

#define CSV

int	    NowYear;		// 2024- 2029
int	    NowMonth;		// 0 - 11

float	NowPrecip;		// inches of rain per month
float	NowTemp;		// temperature this month
float	NowHeight;		// grain height in inches
int	    NowNumDeer;		// number of deer in the current population

int 	increaseDeerBy = 1; // number of deer to increase (changed by MyAgent Function)

const float GRAIN_GROWS_PER_MONTH =	       12.0;
const float ONE_DEER_EATS_PER_MONTH =		1.0;

const float AVG_PRECIP_PER_MONTH =			7.0;	// average
const float AMP_PRECIP_PER_MONTH =			6.0;	// plus or minus
const float RANDOM_PRECIP =					2.0;	// plus or minus noise

const float AVG_TEMP =						60.0;	// average
const float AMP_TEMP =						20.0;	// plus or minus
const float RANDOM_TEMP =					10.0;	// plus or minus noise

const float MIDTEMP =						40.0;
const float MIDPRECIP =						10.0;

const int 	START_YEAR = 					2024;

omp_lock_t	Lock;
volatile int	NumInThreadTeam;
volatile int	NumAtBarrier;
volatile int	NumGone;

void temp_precip (int currMonth);
float SQR( float x );
float calcTempFactor();
float calcPrecipFactor();

void Deer();
void Grain();
void Watcher();
void MyAgent();

void InitBarrier( int );
void WaitBarrier( );
float Ranf( float low, float high );
void calcTempPrecip();

int main () {

	srand (time(NULL));

	calcTempPrecip();

	// starting date and time:
	NowMonth =    0;
	NowYear  = START_YEAR;

	// starting state (feel free to change this if you want):
	NowNumDeer = 2;
	NowHeight =  5.;

	omp_set_num_threads( 4 );	// same as # of sections
	InitBarrier(4);
	#pragma omp parallel sections 
	{
		#pragma omp section 
		{
			Deer();
		}

		#pragma omp section 
		{
			Grain();
		}

		#pragma omp section 
		{
			Watcher();
		}

		#pragma omp section 
		{
			MyAgent( );	// your own
		}
	}       // implied barrier -- all functions must return in order
	// to allow any of them to get past here

	return 0;
}

void temp_precip (int currMonth) {
	float ang = (  30.*(float)NowMonth + 15.  ) * ( M_PI / 180. );	// angle of earth around the sun

	float temp = AVG_TEMP - AMP_TEMP * cos( ang );
	NowTemp = temp + Ranf( -RANDOM_TEMP, RANDOM_TEMP );

	float precip = AVG_PRECIP_PER_MONTH + AMP_PRECIP_PER_MONTH * sin( ang );
	NowPrecip = precip + Ranf( -RANDOM_PRECIP, RANDOM_PRECIP );
	if( NowPrecip < 0. )
		NowPrecip = 0.;
}



float SQR( float x ) {
    return x*x;
}

void calcTempPrecip() {
	float ang = (  30.*(float)NowMonth + 15.  ) * ( M_PI / 180. );	// angle of earth around the sun

	float temp = AVG_TEMP - AMP_TEMP * cos( ang );
	NowTemp = temp + Ranf( -RANDOM_TEMP, RANDOM_TEMP );

	float precip = AVG_PRECIP_PER_MONTH + AMP_PRECIP_PER_MONTH * sin( ang );
	NowPrecip = precip + Ranf( -RANDOM_PRECIP, RANDOM_PRECIP );
	if( NowPrecip < 0. )
		NowPrecip = 0.;

}

float calcTempFactor() {
	/* printf("inside the calctempfactor function: %lf\n", NowTemp - MIDTEMP); */
	return exp(   -SQR(  ( NowTemp - MIDTEMP ) / 10.  )   );
}

float calcPrecipFactor() {
	return exp(   -SQR(  ( NowPrecip - MIDPRECIP ) / 10.  )   );
}

void Deer() {
	while( NowYear < 2030 )
	{
		// compute a temporary next-value for this quantity
		// based on the current state of the simulation:
		int nextNumDeer = NowNumDeer;
		int carryingCapacity = (int)( NowHeight );

		if( nextNumDeer < carryingCapacity ) {
			nextNumDeer+=increaseDeerBy;
		} else {
			if( nextNumDeer > carryingCapacity )
				nextNumDeer--;
		}

		if( nextNumDeer < 0 )
			nextNumDeer = 0;

		// DoneComputing barrier:
		WaitBarrier( );

		NowNumDeer = nextNumDeer;

		// DoneAssigning barrier:
		WaitBarrier( );

		// DonePrinting barrier:
		WaitBarrier( );
	}
}

void Grain() {
	
	while( NowYear < 2030 )
	{
		// compute a temporary next-value for this quantity
		// based on the current state of the simulation:

		calcTempPrecip();
		float nextHeight = NowHeight;
		
		nextHeight += calcTempFactor() * calcPrecipFactor() * GRAIN_GROWS_PER_MONTH;
		nextHeight -= (float)NowNumDeer * ONE_DEER_EATS_PER_MONTH;
		
		
		if( nextHeight < 0. ) nextHeight = 0.;

		NowHeight = nextHeight;

		// DoneComputing barrier:
		WaitBarrier( );

		NowHeight = nextHeight;

		// DoneAssigning barrier:
		WaitBarrier( );

		// DonePrinting barrier:
		WaitBarrier( );
	}
}



void Watcher() {
	while( NowYear < 2030 ) {
		// compute a temporary next-value for this quantity
		// based on the current state of the simulation:

		int nextMonth = NowMonth;
		int nextYear = NowYear;

		if (nextMonth - (NowYear - START_YEAR) * 12 == 11) {
			nextYear++;
			nextMonth++;
		} else {
			nextMonth++;
		}
		

		// DoneComputing barrier:
		WaitBarrier( );

		NowMonth = nextMonth;
		NowYear = nextYear;

		// DoneAssigning barrier:
		WaitBarrier( );

		fprintf(stderr, "%lf, %lf, %d, %lf, %d, %d\n", NowTemp, NowPrecip, NowNumDeer, NowHeight, increaseDeerBy, NowMonth-1);

		// DonePrinting barrier:
		WaitBarrier( );
	}
}

/**
 * Rapid Reproduction!
 * There is a 50% chance that the deer have rapid reproduction!
 * This sets the deer increment variable to either 1 or 2.
*/
void MyAgent() { 
	while( NowYear < 2030 )
	{
		// compute a temporary next-value for this quantity
		// based on the current state of the simulation:
		int random = (int)Ranf(0, 10);

		if (random % 2) {
			increaseDeerBy = 2;
		} else {
			increaseDeerBy = 1;
		}

		// DoneComputing barrier:
		WaitBarrier( );
		
		// DoneAssigning barrier:
		WaitBarrier( );
		
		// DonePrinting barrier:
		WaitBarrier( );
	}
}

void InitBarrier( int n ) {
	NumInThreadTeam = n;
    NumAtBarrier = 0;
	omp_init_lock( &Lock );
}

// have the calling thread wait here until all the other threads catch up:
void WaitBarrier( ) {
        omp_set_lock( &Lock );
        {
                NumAtBarrier++;
                if( NumAtBarrier == NumInThreadTeam )
                {
                        NumGone = 0;
                        NumAtBarrier = 0;
                        // let all other threads get back to what they were doing
			// before this one unlocks, knowing that they might immediately
			// call WaitBarrier( ) again:
                        while( NumGone != NumInThreadTeam-1 );
                        omp_unset_lock( &Lock );
                        return;
                }
        }
        omp_unset_lock( &Lock );

        while( NumAtBarrier != 0 );	// this waits for the nth thread to arrive

        #pragma omp atomic
        NumGone++;			// this flags how many threads have returned
}



float Ranf( float low, float high ) {
	float r = (float) rand();               // 0 - RAND_MAX
	float t = r  /  (float) RAND_MAX;       // 0. - 1.

	return   low  +  t * ( high - low );
}