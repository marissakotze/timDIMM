#!/usr/bin/env python
from hrstar import load_catalog
from pygto900 import GTO900, slew
from astropy.coordinates import Angle
from hrstar_with_precess import Apply_precess

#def goto_hr(current_file='current_object', hr_file='star.lst'):
def goto_hr(Year_NOW,current_file='current_object', hr_file='star.lst'):
    """Go to the current object"""

    try:
       sid = open(current_file).read().strip()
    except Exception, e:
       print e
       return 

    #load catalog
    star_dict=load_catalog(hr_file)

    #get ra/dec for best object
    ra = star_dict[sid][1]
    dec = star_dict[sid][2]
    ra = Angle(ra, unit='hour')
    dec = Angle(dec, unit='degree')
    RA_now, Dec_now = Apply_precess(ra,dec,Year_NOW)

    #slew telescope to position
    with GTO900()  as g:
       print 'Move to: RA(2000)=%s Dec(2000)=%s - RA(NOW)=%s Dec(NOW)=%s'%(ra, dec, RA_now, Dec_now)
       #slew(g, ra, dec)
       slew(g, RA_now, Dec_now)

    
if __name__=='__main__':
   #goto_hr()
   import sys
   goto_hr(int(sys.argv[1]),current_file='current_object', hr_file='star.lst')
