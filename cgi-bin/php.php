#!/usr/bin/php-cgi
<?php

// Required header for CGI
echo "Content-Type: text/html\r\n\r\n";

// Basic output
echo "<html><body>";
echo "<h1>Hello from PHP CGI!</h1>";

// Show request method
echo "<p>Request Method: " . $_SERVER['REQUEST_METHOD'] . "</p>";

// Show query string
echo "<p>Query String: " . $_SERVER['QUERY_STRING'] . "</p>";

// Show some CGI environment variables
echo "<h2>Server Info:</h2>";
echo "<ul>";
echo "<li>Script Name: " . $_SERVER['SCRIPT_NAME'] . "</li>";
echo "<li>Server Protocol: " . $_SERVER['SERVER_PROTOCOL'] . "</li>";
echo "<li>Remote Addr: " . $_SERVER['REMOTE_ADDR'] . "</li>";
echo "</ul>";

echo "</body></html>";
?>