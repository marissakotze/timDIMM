#!/usr/bin/env ruby

require 'GTO900'
require 'ast_utils'
require 'parsedate'

s = GTO900.new()

s.clear
s.clear
s.clear

r = s.ra
d = s.dec
az = s.az
alt = s.alt
side = s.pier?
time = s.get_local_time
date = s.get_local_date

puts "At RA = %s, Dec = %s, Alt = %s, Az = %s, on the %s side of the pier" % [r, d, alt, az, side]

ra_deg = hms2deg(r)
offset = hms2deg("00:01:13")
new = ra_deg + offset
r_new = sexagesimal(new).gsub("+", "")
puts "Actually pointing at RA = %s..." % r_new

rh, rm, rs = r_new.split(':')
dd, dm, ds = d.split(':')
s.command_ra(rh.to_i, rm.to_i, rs.to_i)
s.command_dec(dd.to_i, dm.to_i, ds.to_i)
sleep(1)
s.sync
puts "...synced to RA = %s, Dec = %s" % [r_new, d]

s.close
