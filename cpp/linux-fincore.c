
#include <ctype.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <math.h>
#include <sys/ioctl.h> 
#include <string.h>
#include <dirent.h>
#include <errno.h>

#include "log.h"

#define LINUX_FTOOLS_VERSION "1.3.0"


char STR_FORMAT[] =  "%-80s %18s %18s %18s %18s %18s %18s\n";


long DEFAULT_NR_REGIONS       = 160;  // default number of regions

// program options 
int arg_summarize             = 0;    // print a summary at the end.
int arg_verbose               = 0;    // level of verbosity.
int arg_vertical              = 0;    // print variables vertical
int arg_print_header          = 1;    // print header default


struct fincore_result 
{
    long size;
    long cached_size;
    long pages;
    long cached_pages;
    long min_cached_page;
};

void add_fincore_result(fincore_result *result, const fincore_result *b)
{
    result->size += b->size;
    result->cached_size += b->cached_size;
    result->pages += b->pages;
    result->cached_pages += b->cached_pages;
}


char *_itoa(int n) {
	static char retbuf[100];
	sprintf(retbuf, "%d", n);
	return retbuf;
}

char *_ltoa( long value ) {

    static char buff[100];

    sprintf( buff, "%ld", value );

    return buff;

}

char *_dtoa( double value ) {

    static char buff[100];

    sprintf( buff, "%f", value );

    return buff;

}

double perc( long val, int range ) {
    
    if ( range == 0 )
        return 0;

    double result = ( val / (double)range ) * 100;

    return result;

}

void show_result(const char *path, const fincore_result *result)
{
    float cached_perc = 0.0f;
    if (result->pages > 0 && result->cached_pages > 0) {
        cached_perc = (float)(result->cached_pages) / result->pages;
    }
    
    if ( arg_vertical ) {
        if (arg_verbose) {
            printf("%s\n", path);
            printf("size: %ld\n", result->size);
            printf("cached_size: %ld\n", result->cached_size);
                
            printf("total_pages: %ld\n", result->pages);
            printf("min_cached_page: %ld\n", result->min_cached_page);
            printf("cached: %ld\n", result->cached_pages);
            printf("cached_perc: %.2f\n", cached_perc);
                
            printf("\n");
        }
        else {
            printf("%s\n", path);
            printf("size: %ld\n", result->size);
            printf("cached_size: %ld\n", result->cached_size);
            printf("cached_perc: %.2f\n", cached_perc);
        }
    } else {
        if (arg_print_header == 1) {
            if (arg_verbose) {
                printf("%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
                       "filename",
                       "size",
                       "cached_size",
                       "total_pages",
                       "cached_pages",
                       "min_cached page",
                       "cached_perc");
            }
            else {
                printf("%s\t%s\t%s\t%s\n",
                       "filename",
                       "size",
                       "cached_size",
                       "cached_perc");
            }
            
            arg_print_header = 0;
        }
        
        if (arg_verbose) {
            printf("%s\t%ld\t%ld\t%ld\t%ld\t%ld\t%.2f\n",
                   path, 
                   result->size ,
                   result->cached_size,
                   result->pages,
                   result->cached_pages,
                   result->min_cached_page,
                   cached_perc );
        }
        else  {
            printf("%s\t%ld\t%ld\t%.2f\n\n",
                   path,
                   result->size,
                   result->cached_size,
                   cached_perc);
        }
    }
}

int fincore(char* path, struct fincore_result *result )
{
    int ret = -1;
    
    int fd;
    struct stat file_stat;
    void *file_mmap;

    // vector result from mincore
    unsigned char *mincore_vec;

    // default page size of this OS
    size_t page_size = getpagesize();
    size_t page_index;

    // the number of pages that we have detected that are cached 
    size_t cached = 0;

    // the number of pages that we have printed 
    size_t printed = 0;

    //the total number of pages we're working with
    size_t total_pages;

    //the oldest page in the cache or -1 if it isn't in the cache.
    off_t min_cached_page = -1 ;

    size_t calloc_size = 0;

    size_t cached_size = 0;
    double cached_perc = 0.0f;

    // by default the cached size is zero so initialize this member.
    result->cached_size = 0;

    fd = open( path, O_RDONLY );

    if ( fd == -1 ) {
        TERR("Could not open file[%s]: %s", path, strerror(errno));
        goto cleanup;
    }

    if ( fstat( fd, &file_stat ) < 0 ) {
        TERR("Could not stat file[%s]: %s", path, strerror(errno));
        goto cleanup;

    }

    if ( file_stat.st_size == 0 ) {
        // this file could not have been cached as it's empty so anything we
        //would do is pointless.
        goto cleanup;
    }

    file_mmap = mmap((void *)0, file_stat.st_size, PROT_NONE, MAP_SHARED, fd, 0 );
    
    if ( file_mmap == MAP_FAILED ) {
        TERR("Could not mmap file[%s]: %s", path, strerror(errno));
        goto cleanup;      
    }

    calloc_size = (file_stat.st_size+page_size-1)/page_size;

    mincore_vec = (unsigned char *)calloc(1, calloc_size);

    if ( mincore_vec == NULL ) {
        TERR("Could not calloc: %s", strerror(errno));
        goto cleanup;
    }

    if ( mincore(file_mmap, file_stat.st_size, mincore_vec) != 0 ) {
        TERR("Could not call mincore on file[%s]: %s", path, strerror(errno));
        goto cleanup;
    }

    total_pages = (int)ceil( (double)file_stat.st_size / (double)page_size );

    for (page_index = 0; page_index <= file_stat.st_size/page_size; page_index++) {

        if (mincore_vec[page_index]&1) {

            ++cached;

            if ( min_cached_page == -1 || page_index < min_cached_page ) {
                min_cached_page = page_index;
            }
        }

    }

    if ( printed ) printf("\n");

    cached_perc = 100 * (cached / (double)total_pages); 

    cached_size = (long)cached * (long)page_size;

    result->size = file_stat.st_size;
    result->cached_size = cached_size;
    result->pages = total_pages;
    result->cached_pages = cached;
    result->min_cached_page = min_cached_page;


    show_result(path, result);

    ret = 0;

cleanup:

    if ( mincore_vec != NULL ) {
        free(mincore_vec);
        mincore_vec = NULL;
    }

    if ( file_mmap != MAP_FAILED )
        munmap(file_mmap, file_stat.st_size);

    if ( fd != -1 )
        close(fd);

    return ret;
}

int fincore_file(char *path, struct fincore_result *result);
int fincore_reg(char *path, fincore_result *result);
int fincore_dir(char *path, fincore_result *result);

int fincore_reg(char *path, fincore_result *result)
{
    int ret = 0;
    struct fincore_result fr;
    memset(&fr, 0, sizeof(fr));
    ret = fincore(path, &fr);
    if (ret == 0) {
        add_fincore_result(result, &fr);
    }
    else {
        TERR("failed to fincore regular file[%s]\n", path);
    }
    
    return ret;
}

int fincore_dir(char *path, fincore_result *result)
{
    DIR *dir = opendir(path);
    if (dir == NULL) {
        TERR("failed to open directory[%s]: %s", path, strerror(errno));
        return -1;
    }

    int ret = 0;
    char filename[PATH_MAX];
    struct dirent *ent = NULL;
    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_name[0] == '.') {
            continue;
        }
        
        snprintf(filename, PATH_MAX, "%s/%s", path, ent->d_name);
        
        fincore_result fr;
        memset(&fr, 0, sizeof(fr));
        ret = fincore_file(filename, &fr);
        if (ret != 0) {
            TERR("failed to fincore_file[%s]", filename);
            continue;
        }
        
        add_fincore_result(result, &fr);
    }
    
    closedir(dir);
    return 0;
}

int fincore_file(char *path, struct fincore_result *result)
{
    int ret = 0;

    struct stat st;
    if (stat(path, &st) != 0) {
        TERR("failed to stat file[%s]: %s", path, strerror(errno));
        return -1;
    }

    fincore_result fr;
    memset(&fr, 0, sizeof(fr));
    
    if (S_ISREG(st.st_mode)) {
        ret = fincore_reg(path, &fr);
    }
    else if (S_ISDIR(st.st_mode)) {
        ret = fincore_dir(path, &fr);
    }
    else {
        TERR("unsupported file type[%s]\n", path);
        return -1;
    }
    
    if (ret != 0) {
        TERR("failed to fincore path[%s]\n", path);
        return ret;
    }

    add_fincore_result(result, &fr);

    return 0;
}


// print help / usage
void help() {

    fprintf( stderr, "%s version %s\n", "fincore", LINUX_FTOOLS_VERSION );
    fprintf( stderr, "fincore [options] files...\n" );
    fprintf( stderr, "\n" );

    fprintf( stderr, "  -s --summarize          When comparing multiple files, print a summary report\n" );
    fprintf( stderr, "  -L --vertical           Print the output of this script vertically.\n" );
    fprintf( stderr, "  -v --verbose            Print extra information.\n");
    fprintf( stderr, "  -H --header             Print the header line, only non vertical mode.\n" );
             
    fprintf( stderr, "  -h --help               Print this message.\n" );
    fprintf( stderr, "\n" );

}

/**
 * see README
 */
int main(int argc, char *argv[]) {

    int c;

    static struct option long_options[] =
        {
            /* These options don't set a flag.
               We distinguish them by their indices. */
            {"summarize",         no_argument,             0, 's'},
            {"vertical",          no_argument,             0, 'L'},
            {"verbose",           no_argument,             0, 'v'},
            {"header",            required_argument,       0, 'H'},
            {"help",              no_argument,             0, 'h'},
            {0, 0, 0, 0}
        };

    while (1) {

        /* getopt_long stores the option index here. */
        int option_index = 0;
        
        c = getopt_long (argc, argv, "sLvHh", long_options, &option_index);
        
        /* Detect the end of the options. */
        if (c == -1)
            break;
        
        //bool foo = 1;

        switch (c)
        {
        case 0:
            /* If this option set a flag, do nothing else now. */
            if (long_options[option_index].flag != 0)
                break;
            if (optarg)
                printf (" with arg %s", optarg);
            printf ("\n");
            break;

        case 's':
            arg_summarize = 1;
            break;

        case 'L':
            arg_vertical = 1;
            break;

        case 'v':
            arg_verbose = 1;
            break;

        case 'H':
            if (strcasecmp(optarg, "no") == 0) {
                arg_print_header = 0;
            }
            else {
                arg_print_header = 1;
            }
            break;

        case 'h':
            help();
            return 1;

        case '?':
            /* getopt_long already printed an error message. */
            break;
                
        default:
            fprintf( stderr, "Invalid command line item: %s\n" , argv[ optind ] );
            help();
            return 1;
        }
    } // done processing arguments.

    if ( optind >= argc ) {
        help();
        return 1;
    }

    /* Print any remaining command line arguments (not options). */
        
    struct fincore_result result;
    memset(&result, 0, sizeof(result));
        
    for (; optind < argc; ++optind) {
        if (fincore_file(argv[optind], &result) != 0) {
            TERR("failed to fincore_file[%s]", argv[optind]);
            continue;
        }
    }

    if ( arg_summarize ) {
        printf("---\n");
        
        float cached_perc = 0.0f;
        if (result.pages > 0 && result.cached_pages > 0) {
            cached_perc = (float)(result.cached_pages) / result.pages;
        }

        printf("total size: %ld\n", result.size);
        printf("total cached size: %ld\n", result.cached_size);
        printf("total pages: %ld\n", result.pages);
        printf("total cached pages: %ld\n", result.cached_pages);
        printf("total cache perc: %.2f\n", cached_perc);
    }

    return 0;
}
