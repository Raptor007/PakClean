// PakClean by Raptor007


#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdint.h>
#include <map>
#include <string>


class DataFile
{
public:
	char Name[ 57 ];
	size_t Size;
	void *Data;
	size_t Offset;
	
	DataFile( void )
	{
		memset( Name, 0, 57 );
		Size = 0;
		Data = NULL;
		Offset = 0;
	}
	
	DataFile( const char *name, size_t size, size_t offset )
	{
		memset( Name, 0, 57 );
		strncpy( Name, name, 56 );
		Size = size;
		Data = malloc( std::max<size_t>( 1, size ) );
		if( Data )
			memset( Data, 0, size );
		Offset = offset;
	}
	
	~DataFile()
	{
		free( Data );
		Data = NULL;
	}
	
	bool operator<( const DataFile &other ) const
	{
		return (strcasecmp( Name, other.Name ) < 0);
	}
};


uint32_t GetLE32( const unsigned char *buffer )
{
	return ( (((uint32_t) buffer[ 3 ] ) << 24)
	       | (((uint32_t) buffer[ 2 ] ) << 16)
	       | (((uint32_t) buffer[ 1 ] ) <<  8)
	       |  ((uint32_t) buffer[ 0 ] ) );
}

uint32_t ReadLE32( FILE *in )
{
	unsigned char buffer[ 4 ] = {0,0,0,0};
	fread( buffer, 1, 4, in );
	return GetLE32( buffer );
}

void PutLE32( uint32_t num, unsigned char *buffer )
{
	buffer[ 3 ] = (num & 0xFF000000) >> 24;
	buffer[ 2 ] = (num & 0x00FF0000) >> 16;
	buffer[ 1 ] = (num & 0x0000FF00) >>  8;
	buffer[ 0 ] = (num & 0x000000FF);
}

void WriteLE32( uint32_t num, FILE *out )
{
	unsigned char buffer[ 4 ] = {0,0,0,0};
	PutLE32( num, buffer );
	fwrite( buffer, 1, 4, out );
}


int main( int argc, char **argv )
{
	if( argc < 2 )
	{
		fprintf( stderr, "Usage: %s <filename>\n", argv[ 0 ] );
		return -1;
	}
	
	for( int arg = 1; arg < argc; arg ++ )
	{
		// Open the file for reading.
		FILE *in = fopen( argv[ arg ], "rb" );
		if( ! in )
		{
			fprintf( stderr, "ERROR: Couldn't read: %s\n", argv[ arg ] );
			continue;
		}
		
		// Make sure it's a Pak file.
		char buffer[ 5 ] = {0,0,0,0,0};
		fread( buffer, 1, 4, in );
		if( strcmp( buffer, "PACK" ) != 0 )
			continue;
		
		printf( "Reading: %s\n", argv[ arg ] );
		fflush( stdout );
		
		// Get the directory offset and length (little endian).
		size_t offset = ReadLE32( in );
		size_t length = ReadLE32( in );
		
		// Seek the directory.
		fseek( in, offset, SEEK_SET );
		
		size_t files = length / 64;
		std::map<std::string,DataFile*> contents;
		char name[ 57 ] = {0};
		
		printf( "Reading directory...\n" );
		fflush( stdout );
		
		// Read the directory.
		for( size_t i = 0; i < files; i ++ )
		{
			memset( name, 0, 57 );
			fread( name, 1, 56, in );
			offset = ReadLE32( in );
			length = ReadLE32( in );
			
			if( ! name[0] )
				fprintf( stderr, "WARNING: Empty path in pak directory.\n" );
			if( contents.find(name) != contents.end() )
				fprintf( stderr, "WARNING: Duplicate entry: %s\n", name );
			
			contents[ name ] = new DataFile( name, length, offset );
		}
		
		offset = 12;
		length = 0;
		
		// Read the files.
		for( std::map<std::string,DataFile*>::iterator item = contents.begin(); item != contents.end(); item ++ )
		{
			if( ! item->second->Data )
			{
				fprintf( stderr, "ERROR: Couldn't allocate for: %s\n", item->second->Name );
				continue;
			}
			
			printf( "Reading file: %s\n", item->second->Name );
			fflush( stdout );
			
			if( fseek( in, item->second->Offset, SEEK_SET ) )
			{
				fprintf( stderr, "ERROR: Couldn't seek to: %s\n", item->second->Name );
				free( item->second->Data );
				item->second->Data = NULL;
				continue;
			}
			
			fread( item->second->Data, 1, item->second->Size, in );
			
			offset += item->second->Size;
			length += 64;
		}
		
		// Open the new file for writing.
		std::string out_name = std::string(argv[ arg ]) + std::string(".pak");
		FILE *out = fopen( out_name.c_str(), "wb" );
		if( ! out )
		{
			fclose( in );
			in = NULL;
			fprintf( stderr, "ERROR: Couldn't write: %s\n", out_name.c_str() );
			continue;
		}
		
		printf( "Writing: %s\n", out_name.c_str() );
		fflush( stdout );
		
		// Start with Pak header.
		fwrite( "PACK", 1, 4, out );
		WriteLE32( offset, out );
		WriteLE32( length, out );
		
		// Write file data.
		for( std::map<std::string,DataFile*>::iterator item = contents.begin(); item != contents.end(); item ++ )
		{
			if( ! item->second->Data )
				continue;
			
			printf( "Writing file: %s\n", item->second->Name );
			fflush( stdout );
			
			item->second->Offset = ftell( out );
			fwrite( item->second->Data, 1, item->second->Size, out );
		}
		
		printf( "Writing directory...\n", out_name.c_str() );
		fflush( stdout );
		
		// Write the directory at the end.
		for( std::map<std::string,DataFile*>::const_iterator item = contents.begin(); item != contents.end(); item ++ )
		{
			fwrite( item->second->Name, 1, 56, out );
			WriteLE32( item->second->Offset, out );
			WriteLE32( item->second->Size, out );
		}
		
		fclose( in );
		in = NULL;
		fflush( out );
		fclose( out );
		out = NULL;
		
		printf( "Done.\n", out_name.c_str() );
		fflush( stdout );
		
		// Memory cleanup between files.
		for( std::map<std::string,DataFile*>::iterator item = contents.begin(); item != contents.end(); item ++ )
			delete item->second;
	}
	
	return 0;
}
