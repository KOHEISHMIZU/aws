#ifndef _CH_AWS1_CTRL_H_
#define _CH_AWS1_CTRL_H_
// Copyright(c) 2016 Yohei Matsumoto, All right reserved. 

// ch_aws1_ctrl.h is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// ch_aws1_ctrl.h is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with ch_aws1_ctrl.h.  If not, see <http://www.gnu.org/licenses/>.

#include "ch_base.h"

enum e_aws1_ctrl_src{
  ACS_UI, ACS_RMT, ACS_AP1, ACS_AP2, ACS_FSET, ACS_NONE
};

extern const char * str_aws1_ctrl_src[ACS_NONE];

struct s_aws1_ctrl_inst{
 // current time
  long long tcur;

  // control modes
  e_aws1_ctrl_src ctrl_src;

  // aws control input (from aws)
  unsigned char rud_aws;
  unsigned char meng_aws;
  unsigned char seng_aws;

  s_aws1_ctrl_inst(): tcur(0), ctrl_src(ACS_UI), 
    rud_aws(127), meng_aws(127), seng_aws(127)
  {
  }
};

struct s_aws1_ctrl_stat{
  // current time
  long long tcur;

  // control output
  unsigned char rud;
  unsigned char meng;
  unsigned char seng;

  // control modes
  e_aws1_ctrl_src ctrl_src;

  // aws control input (from aws)
  unsigned char rud_aws;
  unsigned char meng_aws;
  unsigned char seng_aws;

  // aws remote controller's input (from adc)
  unsigned char rud_rmc;
  unsigned char meng_rmc;
  unsigned char seng_rmc;

  // rudder angle 
  unsigned char rud_sta;     // from adc
  unsigned char rud_sta_out; // to digi-pot

  // remote controller's values corresponding positions 
  unsigned char meng_max_rmc; 
  unsigned char meng_nuf_rmc;
  unsigned char meng_nut_rmc;
  unsigned char meng_nub_rmc;
  unsigned char meng_min_rmc;

  unsigned char seng_max_rmc;
  unsigned char seng_nuf_rmc;
  unsigned char seng_nut_rmc;
  unsigned char seng_nub_rmc;
  unsigned char seng_min_rmc;

  unsigned char rud_max_rmc;
  unsigned char rud_nut_rmc;
  unsigned char rud_min_rmc;

  unsigned char rud_sta_max;
  unsigned char rud_sta_nut;
  unsigned char rud_sta_min;

  // Threashold values of digital potentiometer's
  unsigned char meng_max;
  unsigned char meng_nuf;
  unsigned char meng_nut;
  unsigned char meng_nub;
  unsigned char meng_min;

  unsigned char seng_max;
  unsigned char seng_nuf;
  unsigned char seng_nut;
  unsigned char seng_nub;
  unsigned char seng_min;

  unsigned char rud_max;
  unsigned char rud_nut;
  unsigned char rud_min;

  unsigned char rud_sta_out_max;
  unsigned char rud_sta_out_nut;
  unsigned char rud_sta_out_min;

s_aws1_ctrl_stat():
  ctrl_src(ACS_UI),
    meng_max_rmc(0x81), meng_nuf_rmc(0x80),  meng_nut_rmc(0x7f),  
    meng_nub_rmc(0x7e), meng_min_rmc(0x7d),  
    seng_max_rmc(0x81),  seng_nuf_rmc(0x80), seng_nut_rmc(0x7f),
    seng_nub_rmc(0x7e),  seng_min_rmc(0x7d),
    rud_max_rmc(0x80),  rud_nut_rmc(0x7f),  rud_min_rmc(0x7e),
    rud_sta_max(0xff), rud_sta_nut(0x7f), rud_sta_min(0x00),
    meng(0x7f),  seng(0x7f),  rud(0x7f),  
    meng_max(0x81),meng_nuf(0x80),  meng_nut(0x7f),  
    meng_nub(0x7e), meng_min(0x7d),  
    seng_max(0x81),  seng_nuf(0x80), seng_nut(0x7f),
    seng_nub(0x7e),  seng_min(0x7d),
    rud_max(0x80),  rud_nut(0x7f),  rud_min(0x7e),
    rud_sta_out_max(0xff), rud_sta_out_nut(0x7f), rud_sta_out_min(0x00)
  {
  }
};

class ch_aws1_ctrl_inst: public ch_base
{
 protected:
  s_aws1_ctrl_inst inst;
  long long m_tfile;
 public:
 ch_aws1_ctrl_inst(const char * name): ch_base(name), m_tfile(0)
    {
    }
  
  void set(const s_aws1_ctrl_inst & _inst){
    lock();
    inst = _inst;
    unlock();
  }
  
  void get(s_aws1_ctrl_inst & _inst){
    lock();
    _inst = inst;
    unlock();
  }
  
  virtual size_t get_dsize()
  {
    return sizeof(s_aws1_ctrl_inst);
  }
  
  virtual size_t write_buf(const char * buf)
  {
    lock();
    memcpy((void*)&inst, (void*) buf, sizeof(s_aws1_ctrl_inst));
    unlock();
    return get_dsize();
  }
  
  virtual size_t read_buf(char * buf)
  {
    lock();
    memcpy((void*) buf, (void*)&inst, sizeof(s_aws1_ctrl_inst));
    unlock();
    return get_dsize();
  }
  
  virtual void print(ostream & out)
  {
    out << "channel " << m_name 
	<< " rud " <<  (int) inst.rud_aws 
	<< " meng "<<  (int) inst.meng_aws 
	<< " seng "<< (int) inst.seng_aws 
	<< endl;
  }
  
  
  // file writer method
  virtual int write(FILE * pf)
  {
    if(m_tfile == inst.tcur){
      return 0;
    }
    
    int sz = 0;
    if(pf){
      lock();
      fwrite((void*)&inst.tcur, sizeof(long long), 1, pf);
      fwrite((void*)&inst.ctrl_src, sizeof(e_aws1_ctrl_src), 1, pf);
      fwrite((void*)&inst.rud_aws, sizeof(unsigned char), 1, pf);
      fwrite((void*)&inst.meng_aws, sizeof(unsigned char), 1, pf);
      fwrite((void*)&inst.seng_aws, sizeof(unsigned char), 1, pf);
      sz = sizeof(long long) + sizeof(e_aws1_ctrl_src), sizeof(unsigned char) * 3;
      m_tfile = inst.tcur;
      unlock();
    }
    
    return sz;
  }
  
  // file reader method
  virtual int read(FILE * pf, long long tcur)
  {
    if(!pf)
      return 0;
    
    int sz = 0;
    while(m_tfile <= tcur && !feof(pf)){
      lock();
      size_t res;
      res = fread((void*)&inst.tcur, sizeof(long long), 1, pf);
      if(!res)
	goto eof;
      res = fread((void*)&inst.ctrl_src, sizeof(e_aws1_ctrl_src), 1, pf);
      if(!res)
	goto eof;
      res = fread((void*)&inst.rud_aws, sizeof(unsigned char), 1, pf);
      if(!res)
	goto eof;
      res = fread((void*)&inst.meng_aws, sizeof(unsigned char), 1, pf);
      if(!res)
	goto eof;
      res = fread((void*)&inst.seng_aws, sizeof(unsigned char), 1, pf);
      if(!res)
	goto eof;
      sz = sizeof(long long) + sizeof(e_aws1_ctrl_src), sizeof(unsigned char) * 3;
      m_tfile = inst.tcur;
      unlock();			
    }
    return 0;
  eof:
    unlock();
    return 0;
  }
};

class ch_aws1_ctrl_stat: public ch_base
{
protected:
  s_aws1_ctrl_stat stat;
  long long m_tfile;
public:
 ch_aws1_ctrl_stat(const char * name): ch_base(name), m_tfile(0)
    {
    }
  
  void set(const s_aws1_ctrl_stat & _stat)
  {
    lock();
    stat = _stat;
    unlock();
  }
  
  void get(s_aws1_ctrl_stat & _stat)
  {
    lock();
    _stat = stat;
    unlock();
  }
  
  virtual size_t get_dsize()
  {
    return sizeof(s_aws1_ctrl_stat);
  }
  
  virtual size_t write_buf(const char * buf)
  {
    lock();
    memcpy((void*)&stat, (void*)buf, sizeof(s_aws1_ctrl_stat));
    unlock();
    return get_dsize();
  }
  
  virtual size_t read_buf(char * buf){
    lock();
    memcpy((void*)buf, (void*)&stat, sizeof(s_aws1_ctrl_stat));
    unlock();
    return get_dsize();
  }
  
  virtual void print(ostream & out)
  {
    out << "channel " << m_name << " rud " <<  (int) stat.rud  << " " 
	<<  (int) stat.rud_aws << " meng " <<  (int) stat.meng << " " 
	<<  (int) stat.meng_aws << " seng " <<  (int) stat.seng << " " << (int) stat.seng_aws << " rud_sta " 
	<<  (int) stat.rud_sta << " rud_sta_out " <<  (int) stat.rud_sta_out << endl;
  }
  
  // file writer method
  virtual int write(FILE * pf)
  {
    if(m_tfile == stat.tcur){
      return 0;
    }
    
    int sz = 0;
    if(pf){
      lock();
      fwrite((void*)&stat, sizeof(stat), 1, pf);
      sz = sizeof(stat);
      m_tfile = stat.tcur;
      unlock();
    }
    
    return sz;
  }
  
  // file reader method
  virtual int read(FILE * pf, long long tcur)
  {
    if(!pf)
      return 0;
    
    int sz = 0;
    while(m_tfile <= tcur && !feof(pf)){
      lock();
      size_t res = fread((void*)&stat, sizeof(stat), 1, pf);
      if(!res)
	goto eof;
      sz += res;
      m_tfile = stat.tcur;
      unlock();
    }
    return sz;
  eof:
    unlock();
    return sz;
  }
};
#endif
