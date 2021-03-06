#!/usr/bin/env ruby

require 'GTO900'
require 'ast_utils'

ra, dec = `precess.py #{ARGV[0]} #{ARGV[1]}`.split(' ')

ra_deg = 15.0*hms2deg(ra)
dec_deg = hms2deg(dec)

s = GTO900.new()

s.clear

r = s.ra
l = s.lst

lst = 15.0*hms2deg(l)

h = calc_ha(lst, ra_deg)

if h < 0.0
  puts "Pointing East."

  ih = 0.0
  id = -272.95/3600.0
  np = 193.18/3600.0
  ch = 576.99/3600.0
  me = -165.29/3600.0
  ma = -56.64/3600.0

  ra_deg = ra_deg + ih + ch/Math::cos(dec_deg*Math::PI/180.0) - ma*Math::cos(h*Math::PI/180.0)*Math::tan(dec_deg*Math::PI/180.0) + me*Math::sin(h*Math::PI/180.0)*Math::tan(dec_deg*Math::PI/180.0) + np*Math::tan(dec_deg*Math::PI/180.0)
  dec_deg = dec_deg - id - ma*Math::sin(h*Math::PI/180.0) - me*Math::cos(h*Math::PI/180.0)
else
  puts "Pointing West."

  ih = 0.0
  id = -314.65/3600.0
  np = -132.56/3600.00
  ch = -461.46/3600.0
  me = 204.74/3600.0
  ma = -277.38/3600.0

  ra_deg = ra_deg + ih + ch/Math::cos(dec_deg*Math::PI/180.0) - ma*Math::cos(h*Math::PI/180.0)*Math::tan(dec_deg*Math::PI/180.0) + me*Math::sin(h*Math::PI/180.0)*Math::tan(dec_deg*Math::PI/180.0) + np*Math::tan(dec_deg*Math::PI/180.0)
  dec_deg = dec_deg - id - ma*Math::sin(h*Math::PI/180.0) - me*Math::cos(h*Math::PI/180.0)
end

ra = sexagesimal(ra_deg/15.0).sub('+', '') 
dec = sexagesimal(dec_deg)

rh, rm, rs = ra.split(':')
dd, dm, ds = dec.split(':')

s.command_ra(rh.to_i, rm.to_i, rs.to_i)
s.command_dec(dd.to_i, dm.to_i, ds.to_i)
s.slew

loop {
  r = s.ra
  d = s.dec
  az = s.az
  alt = s.alt

  puts "At RA = %s, Dec = %s, Alt = %s, Az = %s" % [r, d, alt, az]

  d_ra = 15.0*Math::cos(hms2deg(d)*Math::PI/180.0)*(hms2deg(r) - hms2deg(ra))
  d_dec = hms2deg(d) - hms2deg(dec)

  puts "\t %f degrees to go in RA, %f degrees to go in Dec...." % [d_ra, d_dec]
  if d_ra.abs < 3.0/60.0 && d_dec.abs < 3.0/60.0
    puts "Done slewing."
    break
  end
  sleep(1)
}

s.close
