/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of Classdesc

  Open source licensed under the MIT license. See LICENSE for details.
*/

/* this is the per-pin data */
struct Cell: graphcode::Object<Cell>
{
  double myValue;
  void update(const Cell&);
  Cell(): myValue(drand()) {}
};
