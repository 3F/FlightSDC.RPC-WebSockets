del CustomLocations.7z
wget -t1 http://nightorion.dyndns.ws/CustomLocations.7z
svn up
7z.exe x customlocations.7z -y
del CustomLocations.7z
svn commit --file customlocations.txt
del GeoIPCountryCSV.zip
wget -N http://geolite.maxmind.com/download/geoip/database/GeoIPCountryCSV.zip
7z.exe x GeoIPCountryCSV.zip -y
del GeoIPCountryCSV.zip
