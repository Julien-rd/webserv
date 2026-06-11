#!/usr/bin/php-cgi
<?php

// Required header for CGI
echo "Content-Type: text/html\r\n\r\n";

// Basic output
echo "<html><body>\n";
echo "<h1>Hello from PHP CGI!</h1>\n";

// Show request method
echo "<p>Request Method: " . $_SERVER['REQUEST_METHOD'] . "</p>\n";

// Show query string
echo "<p>Query String: " . $_SERVER['QUERY_STRING'] . "</p>\n";

// Show some CGI environment variables
echo "<h2>Server Info:</h2>\n";
echo "<ul>\n";
echo "<li>Script Name: " . $_SERVER['SCRIPT_NAME'] . "</li>\n";
echo "<li>Server Protocol: " . $_SERVER['SERVER_PROTOCOL'] . "</li>\n";
echo "<li>Remote Addr: " . $_SERVER['REMOTE_ADDR'] . "</li>\n";
echo "</ul>\n";

echo "</body></html>\n";
?>