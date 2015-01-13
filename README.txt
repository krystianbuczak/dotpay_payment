CONTENTS OF THIS FILE
---------------------
 * Introduction
 * Requirements
 * Installation
 * Configuration

INTRODUCTION
------------
This module provides Dotpay Payment Gateway (http://dotpay.pl/) integration for the
Drupal Payment platform (https://drupal.org/project/payment). Dotpay is polish online
payment service which offers online money transfering gateway supporting almost all
bank accounts in Poland.

This is very early, development version - only for testing purposes. IT WORKS WHEN YOU
LITERALLY SET SOME VALUES. I hope I change this status soon and send this module to
Drupal project's sandbox.

REQUIREMENTS
------------
This module requires the following modules:
 * Payment (https://drupal.org/project/payment)
You need also Dotpay customer account with appropriate ID number.

INSTALLATION
------------
 * Install as you would normally install a contributed drupal module. See:
   https://drupal.org/documentation/install/modules-themes/modules-7
   for further information.

CONFIGURATION
-------------
 * Go to admin/config/services/payment
 * Click on Payment methods
 * Click on Add payment method
 * Click on Dotpay in the list of available payment method types
 * Fill in coresponding fields (ie. Dotpay ID)

Many thanks to Rico van de Vin (ricovandevin Drupal user/1243726) for his payment modules
and code inspirations :)