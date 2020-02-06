using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls.Primitives;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using Microsoft.Win32;

namespace PlanEditor
{
    public class GridPoint
    {
        public enum PointType { Walkable, Wall, Exit, Obstacle }
        public enum SpatialType { Inside, Outside }

        public PointType Type { get; set; }
        public SpatialType SpaType { get; set; }

        public double WallForce { get; set; }
        public double WallForceAngle { get; set; }
        public double WallX { get; set; }
        public double WallY {get; set;}

        public double ExitAngle { get; set; }
        public double ExitForce {get; set;}
        public double ExitX { get; set; }
        public double ExitY { get; set; }

        public int X { get; set; }
        public int Y { get; set; }

        public GridPoint(PointType type)
        {
            Type = type;

            WallForce = 0;
            WallForceAngle = 0;

            ExitForce = 0;
            ExitAngle = 0;
        }

        public override string ToString()
        {
            return "X: " + X + " Y: " + Y;
        }

        public GridPoint clone()
        {
            return new GridPoint(this.Type) { Type = this.Type, ExitAngle = this.ExitAngle, SpaType = this.SpaType, WallForce = this.WallForce, WallForceAngle = this.WallForceAngle, WallX = this.WallX, WallY = this.WallY, ExitForce = this.ExitForce, ExitX = this.ExitX, ExitY = this.ExitY, X = this.X, Y = this.Y };

        }

       
    }
}
