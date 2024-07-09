/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of Classdesc

  Open source licensed under the MIT license. See LICENSE for details.
*/

/* this is the per-pin data */
struct cell: graphcode::Object<cell>
{
  double my_value;
  void update(const cell&);
  cell(): my_value(drand()) {}
};
