#!/usr/bin/perl -w

use warnings;
use strict;
use WWW::Mechanize;

my $fenix_url = "http://www8.garmin.com/support/download_details.jsp?id=5809";
my $fenix_beta_url = "http://www8.garmin.com/support/download_details.jsp?id=6137";
my $fenix_beta = 0;
my $eTrex_url = "http://www8.garmin.com/support/download_details.jsp?id=5553";

my $update_url;
my $update_filename;
my $model_fenix_d2_tactix;
my $update_version;


my $url = $ARGV[0];
if (!defined $url || ("x" eq "x$url")) {
  print "usage get-update.pl url|[fenix|beta|etrex]\n";
  print "fenix: $fenix_url\n";
  print "eTrex: $eTrex_url\n";
  print "beta:  fenix beta updates: $fenix_beta_url\n";
  exit 1;
}

if ($url eq "fenix") {
  $url = $fenix_url;
}
if ($url eq "beta") {
  $url = $fenix_beta_url;
  $fenix_beta = 1;
}
if ($url eq "etrex") {
  $url = $eTrex_url;
}


print "getting data from url: $url\n";

my $m = WWW::Mechanize->new(
			    agent => 'Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:24.0) Gecko/20100101 Firefox/24.0'
);

$m->get($url);
my $c = $m->content;

# http://download.garmin.com/software/fenix_D2_tactix_400.gcd
# beta: http://download.garmin.com/software/fenix_D2_tactix_402Beta.zip
#       http://download.garmin.com/software/fenix_D2_tactix_405Beta.zip

if (0 == $fenix_beta) {
  $c =~ m{(http://download.garmin.com/software/((fenix_(\w*?))_(\d+)\.gcd))}s
    or die "Can't find the update file url\n";
  print "$1, $2, $3, $4, $5\n";


  $update_url      = $1;
  $update_filename = $2;
  $model_fenix_d2_tactix = $3;
  # $4 -> D2_tactix
  $update_version  = $5;
}
else { # find fenix beta download link
  $c =~ m{(http://download.garmin.com/software/(fenix_D2_tactix_(\d+Beta)\.zip))}s
    or die "Can't find the update file url\n";
  print "$1, $2, $3\n";


  $update_url      = $1;
  $update_filename = $2;
  $model_fenix_d2_tactix = "fenix";
  $update_version  = $3;
}

print "version:  $update_version\n";
print "url:      $update_url\n";
print "filename: $update_filename\n";
print "model:    $model_fenix_d2_tactix\n";

$m->get($update_url);
$m->save_content($update_filename);

print "done\n";
