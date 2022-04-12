#!/usr/bin/env perl
use File::Spec;
#use Cwd 'abs_path';
#use strict;
use File::Spec::Functions qw(rel2abs);
use File::Basename;
#print dirname(rel2abs($0));

# in this script, we expect @ARGV[0] to contain the absolute path 
# of a pin tool to be run
$pintool = @ARGV[0];
#$pin = abs_path($0);
$home = dirname(rel2abs($0));
$pin = $home . "/pin/pin";

chomp($pintool);
$localdir = File::Spec->rel2abs( "./" ) ;

#create directory for test produced files
$tempdir = $localdir . "/_temp/";
mkdir($tempdir, 0666);

#create uniquified result directory here
$dirname = localtime();
chomp($dirname);
$dirname =~ s/ /_/g;
$resultdir = $localdir . "/results_" . $dirname . "/";
$outputdir = $localdir . "/outputs_" . $dirname . "/";
mkdir($resultdir, 0777);
mkdir($outputdir, 0777);

$threads = 1;


# Test names basically, an array of arrays. 
# First arg is test name, 
# Second is the directory where the test needs to be executed, mostly ./ works
# Third is the test executable. 
# the remainder are the test args, as would appear in the c argv array
# The first 6 should work but I am not yet sure if the last 6 will work. I need to see what inputs they require and for last two how they get saved to our system.
@testcases = ( 
	    [
         "Black-Scholes",
         "./", 
         "$home/benchmarks/binaries/blackscholes",
         "${threads}",
         "$home/benchmarks/inputs/blackscholes/in_64K.txt", 
         "${outputdir}Black-Scholes_binary_output.txt",
        ],
        [
         "Bodytrack",
         "./", 
         "$home/benchmarks/binaries/bodytrack",
         "$home/benchmarks/inputs/bodytrack/simlarge/sequenceB_4/", 
         "4",
         "1",
         "1000",
         "5",
         "0",
         "1"
        ],
	    [
         "Cholesky",
         "./", 
         "$home/benchmarks/binaries/cholesky",
         "-p1",
         "<",
         "$home/benchmarks/inputs/cholesky/tk29.0",
        ],
	    [
         "Ferret",
         "./", 
         "$home/benchmarks/binaries/ferret",
         "$home/benchmarks/inputs/ferret/simlarge/corel", 
         "lsh",
         "$home/benchmarks/inputs/ferret/simlarge/queries",
         "10",
         "20",
         "1",
         "${outputdir}Ferret_binary_output.txt",
        ],
	    [
         "FFT",
         "./", 
         "$home/benchmarks/binaries/fft",
         "-m16",
         "-p1",
         "${outputdir}Ferret_binary_output.txt",
        ],
        [
         "Fluidanimate",
         "./",
         "$home/benchmarks/binaries/fluidanimate",
         "1",
         "5",
         "$home/benchmarks/inputs/fluidanimate/in_500K.fluid",
        ],
         [
         "Black-Scholes2",
         "./", 
         "$home/benchmarks/binaries/blackscholes",
         "${threads}",
         "$home/benchmarks/inputs/blackscholes/in_10M.txt", 
         "${outputdir}Black-Scholes2_binary_output.txt",
        ],
        [
         "Fluidanimate2",
         "./",
         "$home/benchmarks/binaries/fluidanimate",
         "1",
         "5",
         "$home/benchmarks/inputs/fluidanimate/in_5K.fluid",
        ],
        [
         "Bodytrack2",
         "./", 
         "$home/benchmarks/binaries/bodytrack",
         "$home/benchmarks/inputs/bodytrack/input_native/sequenceB_261/", 
         "4",
         "1",
         "1000",
         "5",
         "0",
         "1"
        ],
        [
         "Ferret2",
         "./", 
         "$home/benchmarks/binaries/ferret",
         "$home/benchmarks/inputs/ferret/input_test/corel", 
         "lsh",
         "$home/benchmarks/inputs/ferret/input_test/queries",
         "10",
         "20",
         "1",
         "${outputdir}Ferret_binary_output2.txt",
        ],
        [ #Make sure to click x on window that opens up to make sure this one stops eventually
         "Raytrace",
         "./",
         "$home/benchmarks/binaries/raytrace",
         "$home/benchmarks/inputs/raytrace/input_simlarge/happy_buddha.obj",
        ],
         [
         "Swaptions",
         "./",
         "$home/benchmarks/binaries/swaptions",
         "10",
         "5",
         "5",
         "$home/benchmarks/inputs/x264/input_test",
         "${outputdir}swaptions_binary_output.txt",
        ],
        [
         "AFI",
         "./",
         "$home/benchmarks/binaries/afi",
         "$home/benchmarks/inputs/afi/F26-A64-D250K_bayes.dom",
         "1000",
         "1",
         "1",
         "${outputdir}afi_binary_output.txt",
        ],
       ); 


#run testcases
for($i = 0; $i < scalar(@testcases); $i=$i+1) {
 #build test command
 $iplus = $i + 1;
 $testname = $testcases[$i][0];
 $numtests = scalar(@testcases);
 print "\n\nTest $iplus of $numtests: $testname\n\n"; 
 $execfile = "";
 $outfile = $resultdir;
 
 #set the executable
 $execdir = $testcases[$i][1];
 $execfile = $testcases[$i][2];
 @splits = split(/\//, $testcases[$i][2]);
 $outfile = $resultdir . @splits[-1];

 #pick up the args
 for $j (3 .. $#{$testcases[$i]}) {
  $execfile = $execfile . " " . $testcases[$i][$j]; 
  # for the outfile, we want just the file name,not the path
  @splits = split(/\//, $testcases[$i][$j]);
  $outfile = $outfile ."___". @splits[-1];
 }

 $outfile = $outfile . ".out";  
 #$outfile = $resultdir . "output.txt";
 #print "results file:\n";
 #print $outfile . "\n";
 
 #clean < and >
 $outfile =~ s/</_/g; 
 $outfile =~ s/>/_/g; 

 # Now execute.
 $tbegin = time();
 #$fullpath = File::Spec->rel2abs( "./pin/pin" ) ;
 #print $fullpath;
 print "Executing: \n\ncd $execdir; $pin -t $pintool -o $outfile -- $execfile\n\n";
 $execout = `cd $execdir; $pin -t $pintool -o $outfile -- $execfile`;
 print "Executable output: \n\n$execout\n\n";
 
 $ttotal = time() - $tbegin;
 print "\n\nTest took approxiamately: $ttotal seconds\n\n"; 

}

#kill any temporary files
print "\n\nCleaning up temporary files\n\n";
@tempfiles = `ls $tempdir`;
for $filename (@tempfiles) {
  chomp($filename);
  print "Unlinking temporary file: ${tempdir}${filename}\n";
  unlink("${tempdir}${filename}");
}

rmdir($tempdir);


