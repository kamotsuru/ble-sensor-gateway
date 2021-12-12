#!/bin/sh
#cd to this script located directory
cd `dirname $0`

#Usage
#case $# in
# 1 ) ;;
# * ) echo "Usage: ${0##*/} syslog_file"; exit 1;;
#esac

#sqlite3 db file name created by this script file name
#dbfile=`echo ${0##*/} | sed -e s/\.[^\.]*$//`.sqlite3
dbfile="/home/pi/sl2sql.sqlite3"

cmd_sqlite3="/usr/bin/sqlite3"

#create db if the above db file doesn't exist
if [ ! -f ${dbfile} ]; then
  echo "${dbfile} doesn't exist!"
  ${cmd_sqlite3} ${dbfile} "CREATE TABLE sens_data (id INTEGER PRIMARY KEY AUTOINCREMENT, device VARCHAR(64), sensId INTEGER, current REAL, busvoltage REAL, current_mA REAL, timestamp TEXT, updated_at TEXT DEFAULT (datetime(CURRENT_TIMESTAMP, 'localtime')));"  
else
  echo "${dbfile} exists!"
fi

#file=$1
file="/var/log/messages"

#get the latest sensor data timestamp in the db
ex_data=`${cmd_sqlite3} ${dbfile} "SELECT max(id), timestamp FROM sens_data;"` 
ex_timestamp=`echo $ex_data | cut -d '|' -f 2`

#read syslog file line by line
flag=0

while read line
do
    app=`echo ${line} | awk '{print $4}'`
    if [ $app ] && [ $app != "esp32" ]; then
        continue
    fi  
    device=`echo ${line} | awk '{print $6}'`
    sensId=`echo ${line} | awk '{print $7}'`
    current=`echo ${line} | awk '{print $8}'`
    busvoltage=`echo ${line} | awk '{print $9}'`
    current_mA=`echo ${line} | awk '{print $10}'`    
    comp=`echo "$current > 128.0" | bc`
    if [ $comp -eq 1 ]; then
	current = `expr $current - 256.0`
    fi
    comp=`echo "$busvoltage > 128.0" | bc`    
    if [ $comp -eq 1 ]; then
       busvoltage = `expr $busvoltage - 256.0`     
    fi
    comp=`echo "$current_mA > 128.0" | bc`    
    if [ $comp -eq 1 ]; then
       current_mA = `expr $current_mA - 256.0`     
    fi       
    timestamp="`echo ${line} | awk '{print $1}'` `echo ${line} | awk '{print $2}'` `echo ${line} | awk '{print $3}'`"
#    echo $device   
#    echo $sensId   
#    echo $current   
#    echo $busvoltage   
#    echo $timestamp   
    if [ `date -d"${ex_timestamp}" +%s` -lt `date -d"${timestamp}" +%s` ]; then
        flag=1
    fi
#insert sensor data
    if [ $flag -eq 1 ]; then
        ${cmd_sqlite3} ${dbfile} "INSERT INTO sens_data(device, sensId, current, busvoltage, current_mA, timestamp) VALUES('${device}', '${sensId}', '${current}', '${busvoltage}', '${current_mA}', '${timestamp}');"	
    fi
done < $file
