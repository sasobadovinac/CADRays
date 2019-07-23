# Script displays properties of different materials available in OCCT

set THE_MATERIALS {brass bronze copper gold pewter plaster plastic silver steel stone shiny_plastic satin metalized neon_gnc chrome aluminium obsidian neon_phc jade charcoal water glass diamond transparent}

# setup 3D viewer content
pload MODELING VISUALIZATION

# Setup 3D viewer
vclear
vinit name=View1 w=128 h=128
vvbo 0
vsetdispmode 1

# Setup view parameters
vcamera -persp
vviewparams -scale 18 -eye 44.49 -0.15 33.93 -at -14.20 -0.15 7.0 -up -0.48 0.00 0.88

# Create a sphere inside the model
psphere ball 14
incmesh ball 0.01
vdisplay -noupdate ball
vsetlocation -noupdate ball 0 0 14
vsetmaterial -noupdate ball plaster

# Display the model and assign material
vdisplay -noupdate ball
vsetmaterial -noupdate ball glass

# Create chessboard-style floor
box tile 10 10 0.1
eval compound [lrepeat 144 tile] tiles
explode tiles
for {set i 0} {$i < 12} {incr i} {
  for {set j 1} {$j <= 12} {incr j} {
    ttranslate tiles_[expr 12 * $i + $j] [expr $i * 10 - 90] [expr $j * 10 - 70] -0.15
    vdisplay -noupdate tiles_[expr 12 * $i + $j]

    vsetmaterial -noupdate tiles_[expr 12 * $i + $j] plaster

    if {($i + $j) % 2 == 0} {
      vbsdf tiles_[expr 12 * $i + $j] -kd 0.85
    } else {
      vbsdf tiles_[expr 12 * $i + $j] -kd 0.45
    }
  }
}

# Configure light sources
vlight del 1
vlight change 0 head 0
vlight change 0 direction -0.25 -1 -1
vlight change 0 sm 0.3
vlight change 0 int 10

# Load environment map
vtextureenv on D:/Projects/Textures/latlong_3_2.jpg

puts "Trying path tracing mode..."
vrenderparams -ray -gi -rayDepth 10

foreach aMatIter $::THE_MATERIALS {
  vsetmaterial ball "$aMatIter"
  vfps 8000
  vdump "D:/Projects/materials/$aMatIter.png"
}
