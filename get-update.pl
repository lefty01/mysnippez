#!/usr/bin/perl -w

use warnings;
use strict;
use WWW::Mechanize;

my $fenix_url = "http://www8.garmin.com/support/download_details.jsp?id=5809";
my $eTrex_url = "http://www8.garmin.com/support/download_details.jsp?id=5553";

my $url = $ARGV[0];
if (!defined $url || ("x" eq "x$url")) {
  print "usage get-update.pl url\n";
  exit 1;
}

print "getting data from url: $url\n";

my $m = WWW::Mechanize->new(
			    agent => 'Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:24.0) Gecko/20100101 Firefox/24.0'
);

$m->get($url);
my $c = $m->content;

$c =~ m{(http://www.garmin.com/software/(fenix_(\d+)\.gcd))}s
  or die "Can't find the update file url\n";

my $update_url      = $1;
my $update_filename = $2;
my $update_version  = $3;

print "version:  $update_version\n";
print "url:      $update_url\n";
print "filename: $update_filename\n";

$m->get($update_url);
$m->save_content($update_filename);

print "done\n";
