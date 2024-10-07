#!/usr/bin/env sh

######################################################################
# @author      : stanleyarn (stanleyarn@$HOSTNAME)
# @file        : generate
# @created     : Tuesday Sep 17, 2024 13:17:56 CEST
#
# @description : 
######################################################################

YEAR=$1

rm -rf $YEAR
mkdir $YEAR

DAYS=("Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday")
