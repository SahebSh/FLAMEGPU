using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;
namespace PlanEditor
{
    public class MapBuilder
    {
        private NavMap _navMap;
        public NavMap NavMap
        {
            get
            {
                return _navMap;
            }

            set
            {
                _navMap = value;
            }
        }

        public MapBuilder(Canvas eCanvas)
        {

        }

        
    }
}
