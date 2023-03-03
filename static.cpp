#include <iostream>   
#include <stdlib.h>   
#include "PIMFuncs.h" 
#include "mpi.h"      

using namespace std;


#define MASTER     0
#define MAX_WIDTH  32000
#define MAX_HEIGHT 32000


struct complex{
  float real;
  float imag;
};


int cal_pixel( complex );
bool valid( int, char *[], int&, int& );
bool inBounds( int, int );
void outputResults( unsigned char **, double, int, int );


int main( int argc, char *argv[] )
{
  
  int comm_sz, my_rank, tag;
  int kill, proc, nrows, totProcs;
  int width, height;
  int row, col, index, greyScaleMod;
  double radius, radSq;
  double startTime, endTime, deltaTime;
  complex c;
  bool go;
  unsigned char *pixels;

  
  radius = 2.0;
  radSq = ( radius * radius );
  tag = 0;
  row = 0;
  width = height = 0;
  kill = -1;
  greyScaleMod = 256;

  
  MPI_Init( &argc, &argv );
  MPI_Comm_size( MPI_COMM_WORLD, &comm_sz );
  MPI_Comm_rank( MPI_COMM_WORLD, &my_rank );

  
  if( !valid( argc, argv, width, height ) )
  {
    MPI_Finalize();
    return 0;
  }

  
  pixels = ( unsigned char * ) calloc( width, sizeof( unsigned char ) );

  
  if( my_rank == MASTER )
  {
    
    unsigned char **p = new unsigned char *[ height ];
    for( index = 0; index < height; index++ )
    {
      p[ index ] = new unsigned char [ width ];
    }

    
    cout << "width x height = " << width << " x " << height << endl;
    startTime = MPI_Wtime();

    
    totProcs = comm_sz - 1; 
    nrows = height;
    proc = 1;
    while( nrows > 0 )
    {
      MPI_Send( &row, 1, MPI_INT, proc, tag, MPI_COMM_WORLD );
      if( !( proc % totProcs ) ) 
      {
        proc = 0;
      }
      row++;   
      proc++;  
      nrows--; 
    }

    
    for( proc = 1; proc < comm_sz; proc++ )
    {
      MPI_Send( &kill, 1, MPI_INT, proc, tag, MPI_COMM_WORLD );
    }

    
    MPI_Status status;
    for( index = 0; index < height; index++ )
    {
      MPI_Recv( pixels,
                width,
                MPI_UNSIGNED_CHAR,
                MPI_ANY_SOURCE,
                MPI_ANY_TAG,
                MPI_COMM_WORLD,
                &status );
      row = status.MPI_TAG;

      
      for( col = 0; col < width; col++ )
      {
        p[ row ][ col ] = pixels[ col ];
      }
    }

    
    endTime = MPI_Wtime();
    deltaTime = endTime - startTime;

    
    outputResults( p, deltaTime, width, height );

    
    for( index = 0; index < height; index++ )
    {
      delete [] p[ index ];
    }
    delete [] p;
  }
  else
  {
    go = true;
    while( go )
    {
      MPI_Recv( &row,
                1,
                MPI_INT,
                MASTER,
                tag,
                MPI_COMM_WORLD,
                MPI_STATUS_IGNORE );
      if( row == -1 )
      {
        go = false;
      }
      else
      {
        for( col = 0; col < width; col++ )
        {
          c.real = ( col - width / radius ) * ( radSq / width );
          c.imag = ( row - height / radius) * ( radSq / width );

          
          pixels[ col ] = ( cal_pixel( c ) * 35 ) % greyScaleMod;
        }
        MPI_Send( pixels,
                  width,
                  MPI_UNSIGNED_CHAR,
                  MASTER,
                  row,
                  MPI_COMM_WORLD );
      }
    }
  }
  
  free( pixels );
  MPI_Barrier( MPI_COMM_WORLD );
  MPI_Finalize();
  return 0;
}


int cal_pixel( complex c )
{
  complex z;
  int count, max;
  float temp, lengthsq;
  max = 256;
  z.real = 0; z.imag = 0;
  count = 0;
  do
  {
    temp = z.real * z.real - z.imag * z.imag + c.real;
    z.imag = 2 * z.real * z.imag + c.imag;
    z.real = temp;
    lengthsq = z.real * z.real + z.imag * z.imag;
    count++;
  } while ( ( lengthsq < 4.0 ) && ( count < max ) );
  return count;
}

bool inBounds( int w, int h )
{
  if( ( w < 0 ) || ( w > MAX_WIDTH ) || ( h < 0 ) || ( h > MAX_HEIGHT ) )
  {
    cout << "ERROR: Invalid image dimensions." << endl;
    cout << "Height and width range: [1,32000]" << endl;
    return false;
  }
  return true;
}

bool valid( int argc, char *argv[], int &w, int &h )
{
  if( argc == 3 )
  {
    w = atoi( argv[ 1 ] );
    h = atoi( argv[ 2 ] );
    if( inBounds(w, h) )
    {
      return true;
    }
  }
  cout << "ERROR: Invalid input." << endl;
  cout << "Arguments should be <width> <height>" << endl;
  cout << "Shutting down..." << endl;
  return false;
}
 
void outputResults( unsigned char ** pixs, double time, int w, int h )
{
  const unsigned char **p;
  p = ( const unsigned char ** ) pixs;
  cout.precision( 6 );
  cout << "Set calculation took " << fixed << time << "s." << endl;
  cout << "Writing image to file 'static.pgm'" << endl;

  if( pim_write_black_and_white( "static.pgm", w, h, p ) )
  {
    cout << "SUCCESS: image written to file." << endl;
  }
  else
  {
    cout << "FAILED: image NOT written to file." << endl;
  }
}