#!/bin/bash
input="data_links.txt"
while IFS= read -r line
do
	trimmed=`echo $line | sed 's/ *$//g'`
	url="https://noaa-nexrad-level2.s3.amazonaws.com/2023/12/06/KABR/$trimmed"
	echo $url
	curl $url --output "data/$trimmed"
done < "$input"

