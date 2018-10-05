function gbcover
%GBCOVER compile GraphBLAS for statement coverage testing
%
% This function compiles all of GraphBLAS in ../Source and all GraphBLAS
% mexFunctions in ../Test, and inserts code for statement coverage testing.
%
% See also: gbcover_edit, gbcmake

%  SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2018, All Rights Reserved.
%  http://suitesparse.com   See GraphBLAS/Doc/License.txt for license.

% create the include files
hfiles = [ ...
           dir('../Source/*.h') ; ...
           dir('../Source/Template') ; ...
           dir('../Source/Generated/*.h') ; ] ;
count = gbcover_edit (hfiles, 0, 'tmp_include') ;
fprintf ('hfile count: %d\n', count) ;

% create the C files
cfiles = [ dir('../Source/*.c') ; ...
           dir('../Source/Generated/*.c') ; ...
           dir('../Demo/Source/usercomplex.c') ; ...
           dir('../Demo/Source/simple_rand.c') ; ...
           dir('../Demo/Source/simple_timer.c') ; ...
           dir('../Demo/Source/random_matrix.c') ; ...
           dir('../Demo/Source/wathen.c') ; ...
           dir('../Demo/Source/mis_check.c') ; ...
           dir('../Demo/Source/mis_score.c') ; ...
           dir('gbcover_finish.c')
           ] ;
count = gbcover_edit (cfiles, count, 'tmp_source') ;
fprintf ('cfile count: %d\n', count) ;

% create the gbcover_finish.c file
f = fopen ('tmp_source/gbcover_finish.c', 'w') ;
fprintf (f, '#include "GB.h"\n') ;
fprintf (f, 'int64_t gbcov [GBCOVER_MAX] ;\n') ;
fprintf (f, 'int gbcover_max = %d ;\n', count) ;
fclose (f) ;

gbcmake ;

