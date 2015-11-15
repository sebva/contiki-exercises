#!/usr/bin/perl -w

use strict;
use IO::Socket;
use IO::Socket::INET6;
use MIME::Lite;


my ($host, $port, $kidpid, $handle, $line);

$host = "aaaa::212:7400:115b:e8e6";
$port = "100";

# create a tcp connection to the specified host and port
$handle = IO::Socket::INET6->new(Proto     => "tcp",
                                PeerAddr  => $host,
                                PeerPort  => $port)
       or die "can't connect to port $port on $host: $!";

$handle->autoflush(1);              # so output gets there right away
print STDERR "[Connected to $host:$port]\n";

# split the program into two processes, identical twins
die "can't fork: $!" unless defined($kidpid = fork());

if ($kidpid) {                      
    # parent copies the socket to standard output
    while (defined ($line = <$handle>)) {
        print STDOUT "RECV from node: " . $line;
	if (index($line, 'lightvaluechange') != -1) {
		# An intruder was detected
		sendEmail();
	}
    }
    kill("TERM" => $kidpid);        # send SIGTERM to child
}
else {                              
    # child copies standard input to the socket
    while (defined ($line = <STDIN>)) {
        print $handle $line;
    }
}
exit;



sub sendEmail {
    my $receiver        = "seb.vaucher\@gmail.com";
    my $subject         = "Light Activity Alert";
    my $message         = "Someone just switched the light in the room with your sensor!\n";
 rss.vaucher.
    system("perl sendEmail.pm -f $receiver -t $receiver -u \"$subject\" -m \"$message\" -s pi.home.vaucher.org:25");
}


