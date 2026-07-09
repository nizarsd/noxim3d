#include "DPNode.h"
#include <algorithm>
#define alpha 1


void DPNode::dpProcess()
{
	// Gating for other selection methods
	if (TGlobalParams::selection_strategy != SEL_DP)  return;
	



	int stime  = (int) (sc_time_stamp().to_double()/1000 - DEFAULT_RESET_TIME);

	
	// One destination held for `dwell` (>= mesh diameter) consecutive cycles so its
	// cost field fully converges in place before advancing to the next destination.
	// measure -> snapshot -> DP converge (config)-> settle for ss measurement 
	int phase = stime % dp_cycle();
	
	if (phase >= dp_pass()) return;              // SETTLE: DP idle
	
	dst_id = (phase / dp_dwell()) % dp_no_dst(); // converge-phase destination
	


	if (reset.read())
	{
		for (int i=0; i<DIRECTIONS; i++)
		{
			dp_tx[i].write  (BIG_VALUE);
			dp_dir[i].write (NOT_VALID);
			frozen_local_cost[i] = 0;
		}
		for (int d=0; d<DPSIZE; d++)
			for (int i=0; i<DIRECTIONS; i++)
				cost_mem[d][i] = BIG_VALUE;
			
		return;
	}

	if (!dp_clock.posedge())  return;
	
	// for single cost metric
	// if (phase == 0) 
		// frozen_local_cost = local_dp_cost.read();
	
	if (phase == 0) {
		for (int i=0; i<DIRECTIONS; i++)
			frozen_local_cost[i] = local_dp_cost[i].read();
	}

	// Destination node: cost anchor (0 to itself), every cycle of its dwell window.
	if (local_id == dst_id)
	{
		for (int i=0; i<DIRECTIONS; i++)
		{
			dp_tx[i].write (0);
			dp_dir[i].write(NOT_VALID);
			cost_mem[dst_id][i] = 0;
		}
		
		#ifdef DP_DEBUG
		if (dst_id == DP_WATCH_DST)
			std::cout << "[DP] dst=" << dst_id << " step=" << (stime % dp_dwell())
					  << " node=" << local_id << " ANCHOR mincost=0" << std::endl;
		#endif
		
		return;
	}

	// RELAX (every cycle of the dwell): dp_rx holds neighbours' stored costs for
	// this dst_id, coherent because all nodes hold the same dst_id for the window.
	int rx_dp_cost[DIRECTIONS];
	for (int i=0; i<DIRECTIONS; i++)
		rx_dp_cost[i] = (dp_rx[i] >= BIG_VALUE) ? BIG_VALUE
		              : (int)(dp_rx[i]*alpha) + frozen_local_cost[i] + 1;

	int sorted_ports[] = {0,1,2,3,4,5};
	BubbleSort(rx_dp_cost, sorted_ports);       // ports ordered by ascending cost

	int dp_cost[DIRECTIONS];
	for (int i=0; i<DIRECTIONS; i++)
		dp_cost[i] = BIG_VALUE;

	// Min cost per input direction j, over legal-turn output ports (i).
	for (int i=0; i<DIRECTIONS; i++)
		for (int j=0; j<DIRECTIONS; j++)
			if (can_turn(j, sorted_ports[i], dst_id) && dp_cost[j] > rx_dp_cost[sorted_ports[i]])
				dp_cost[j] = rx_dp_cost[sorted_ports[i]];

	#ifdef DP_DEBUG
	if (dst_id == DP_WATCH_DST && local_id == 1) {
	std::cout << "can_turn(j, WEST): ";
	for (int j=0;j<DIRECTIONS;j++)
		std::cout << j << "=" << can_turn(j, DIRECTION_WEST, dst_id) << " ";
	std::cout << std::endl;
	}
	#endif
	
	for (int i=0; i<DIRECTIONS; i++)
	{
		dp_tx[i].write (dp_cost[i]);
		// invalidate ranks whose output-port cost is BIG_VALUE (turn-illegal / unreachable)
		if (rx_dp_cost[sorted_ports[i]] >= BIG_VALUE)
			dp_dir[i].write(NOT_VALID);
		else
			dp_dir[i].write(sorted_ports[i]);
		
		cost_mem[dst_id][i] = dp_cost[i];
	}
		
// ---- DEBUG: convergence trace for one fixed destination ----------------
// Compile with -DDP_DEBUG. Traces cost to DP_WATCH_DST only.
#ifdef DP_DEBUG
	if (dst_id == DP_WATCH_DST)
	{
		int mincost = BIG_VALUE;
		for (int i=0; i<DIRECTIONS; i++)
			if (dp_cost[i] < mincost) mincost = dp_cost[i];

		int step = stime % dp_dwell();          // hop index within this dst's window
		int nx = local_id % TGlobalParams::mesh_dim_x;
		int ny = (local_id / TGlobalParams::mesh_dim_x) % TGlobalParams::mesh_dim_y;
		int nz = local_id / (TGlobalParams::mesh_dim_x*TGlobalParams::mesh_dim_y);
		std::cout << "[DP] dst=" << dst_id << " step=" << (stime % dp_dwell())
				  << " node=" << local_id << "(" << nx << "," << ny << "," << nz << ")"
				  << " mincost=" << (mincost>=BIG_VALUE ? -1 : mincost)
				  << " frz[";
		for(int i=0;i<DIRECTIONS;i++) std::cout << frozen_local_cost[i] << " ";
		std::cout << "] rx[";
		for(int i=0;i<DIRECTIONS;i++) std::cout << dp_rx[i].read() << " ";
		std::cout << "] perdir[";
		for(int i=0;i<DIRECTIONS;i++) std::cout << (dp_cost[i]>=BIG_VALUE?-1:dp_cost[i]) << " ";
		std::cout << "]" << std::endl;
		
	}
#endif

}


void  DPNode::configure(const int _local_id)
{
  local_id = _local_id;
 
}

// check if the turn is allowed
bool  DPNode::can_turn(int dir_in, int dir_out, int dst_id)
{

 switch (TGlobalParams::routing_algorithm) {

  case ROUTING_NEGATIVE_FIRST: 
	    return   can_turnNegativeFirst(dir_in, dir_out, dst_id);
	    break;
  case ROUTING_ODD_EVEN: 
	    return	can_turnOddEven(dir_in, dir_out, dst_id);
	    break;
 case ROUTING_DW_ODD_EVEN: 
	    return	can_turnDwOddEven(dir_in, dir_out, dst_id);
	    break;
case ROUTING_ODD_EVEN_3DNM: 
	    return	can_turnOddEvenNM(dir_in, dir_out, dst_id);
	    break;

case ROUTING_ODD_EVEN_BALANCED:
		return can_turnOddEvenBalanced(dir_in, dir_out, dst_id);
		break;
  default:
	return true;
}



}

// 3D Odd Even <Nizar>
bool DPNode::can_turnOddEven(int dir_in, int dir_out, int dst_id)
{
  TCoord current  		= id2Coord(local_id);
  TCoord destination 	= id2Coord(dst_id);
  int idfrom=189, idto=5;
  TCoord source 	= current;

  switch ( dir_in ) {

  case DIRECTION_NORTH : 
	     source.y-=1;
	     break;
  case DIRECTION_EAST : 
	    source.x+=1;
	    break;
  case DIRECTION_SOUTH : 
	    source.y+=1;
	    break;
  case DIRECTION_WEST: 
	    source.x-=1;
	    break;
  case DIRECTION_UP : 
	    source.z+=1;
	    break;
  case DIRECTION_DOWN : 
	    source.z-=1;
	    break;
  default:
	// you must not be here !
	assert (false);
}

   vector<int> directions;
   int sz=source.z;
   int cz=current.z;
   int dz=destination.z;
   
   int sx=source.x;
   int cx=current.x;
   int dx=destination.x;
   
   int sy=source.y;
   int cy=current.y;
   int dy=destination.y;

   int ex = dx-cx;
   int ey = dy-cy;
   int ez = dz-cz;

 if (ez == 0)
	directions=routingOddEven(current, source, destination);

  else
    {
	  if (ez > 0)   // z direction is +ve (going down)
	    {
		if ((ex==0) && (ey == 0))  // at the xy of destination 
			directions.push_back(DIRECTION_DOWN);
	    else
	    	{
	      	 if ((cz % 2 == 1) || (cz==sz))   //
	 		directions=routingOddEven(current, source, destination);
				
  		 if ((dz % 2 == 1) || (ez != 1))
		  	directions.push_back(DIRECTION_DOWN);
		}
		
	    } // z direction is +ve
	  
	  else   // z direction is -ve ( going up)
		{
		  // need xy-plane routing and the z plane is even	
		directions.push_back(DIRECTION_UP);

	        if ((ex!=0 || ey!=0) &&  (cz % 2 == 0) )  
			directions=routingOddEven(current, source, destination);
		

	    } // z direction is -ve
    } //ez==0


bool in_directions=false;

for (int i=0; i<directions.size(); i++)
	if(dir_out==directions[i])
		in_directions=true;


/*if (local_id == idfrom && dst_id==idto)
	cout<<" from: "<<dir_in<< " to: "<< dir_out<<" is: "<<in_directions<<endl; //*/



return in_directions;

 
}


vector<int> DPNode::routingOddEven(const TCoord& current, 
          			    const TCoord& source, const TCoord& destination)
{
  vector<int> directions;
  int c0 = current.x;
  int c1 = current.y;
  int s0 = source.x;
  //  int s1 = source.y;
  int d0 = destination.x;  

  int d1 = destination.y;
  int e0, e1;

  e0 = d0 - c0;
  e1 = -(d1 - c1);

  if (e0 == 0)
    {
      if (e1 > 0)
	directions.push_back(DIRECTION_NORTH);
      else
	directions.push_back(DIRECTION_SOUTH);
    }
  else
    {
      if (e0 > 0)
	{
	  if (e1 == 0)
	    directions.push_back(DIRECTION_EAST);
	  else
	    {
	      if ( (c0 % 2 == 1) || (c0 == s0) )
		{
		  if (e1 > 0)
		    directions.push_back(DIRECTION_NORTH);
		  else
		    directions.push_back(DIRECTION_SOUTH);
		}
	      if ( (d0 % 2 == 1) || (e0 != 1) )
		directions.push_back(DIRECTION_EAST);
	    }
	}
      else
	{
	  directions.push_back(DIRECTION_WEST);
	  if (c0 % 2 == 0)
	    {
	      if (e1 > 0)
		directions.push_back(DIRECTION_NORTH);
	      if (e1 < 0) 
		directions.push_back(DIRECTION_SOUTH);
	    }
	}
    }
  
  if (!(directions.size() > 0 && directions.size() <= 2))
  {
      cout << "\n STAMPACCHIO :";
      cout << source << endl;
      cout << destination << endl;
      cout << current << endl;

  }
  assert(directions.size() > 0 && directions.size() <= 2);
  
  return directions;
}


bool DPNode::can_turnNegativeFirst(int dir_in, int dir_out, int dst_id)
{
 int idfrom=224, idto=25; 
 
 vector<int> directions;
 TCoord current  	= id2Coord(local_id);
 TCoord destination 	= id2Coord(dst_id);



if (destination.x < current.x && destination.y < current.y && destination.z > current.z )
    {
      directions.push_back(DIRECTION_NORTH);
      directions.push_back(DIRECTION_WEST);
      directions.push_back(DIRECTION_DOWN);
    }
  else if (destination.x < current.x && destination.y < current.y && destination.z <= current.z )
    {
      directions.push_back(DIRECTION_NORTH);
      directions.push_back(DIRECTION_WEST);
    }
   else if (destination.x < current.x && destination.y >= current.y && destination.z > current.z )
    {
      directions.push_back(DIRECTION_DOWN);
      directions.push_back(DIRECTION_WEST);
    }
   else if (destination.x >= current.x && destination.y < current.y && destination.z > current.z )
    {
      directions.push_back(DIRECTION_DOWN);
      directions.push_back(DIRECTION_NORTH);
    }
   else if (destination.x < current.x && destination.y >= current.y && destination.z <= current.z )
    {
      directions.push_back(DIRECTION_WEST);
    }
   else if (destination.x >= current.x && destination.y < current.y && destination.z <= current.z )
    {
      directions.push_back(DIRECTION_NORTH);
    }
   else if (destination.x >= current.x && destination.y >= current.y && destination.z > current.z )
    {
      directions.push_back(DIRECTION_DOWN);
    }
   else if (destination.x > current.x && destination.y > current.y && destination.z < current.z )
    {
      directions.push_back(DIRECTION_SOUTH);
      directions.push_back(DIRECTION_EAST);
      directions.push_back(DIRECTION_UP);
    }
   else if (destination.x > current.x && destination.y > current.y && destination.z == current.z )
    {
      directions.push_back(DIRECTION_SOUTH);
      directions.push_back(DIRECTION_EAST);
    }
   else if (destination.x > current.x && destination.y == current.y && destination.z < current.z )
    {
      directions.push_back(DIRECTION_UP);
      directions.push_back(DIRECTION_EAST);
    }
   else if (destination.x == current.x && destination.y > current.y && destination.z < current.z )
    {
      directions.push_back(DIRECTION_UP);
      directions.push_back(DIRECTION_SOUTH);
    }
  else
     directions=routingXYZ(current, destination);


/* if (local_id == idfrom) //&& dst_id==idto) 
{
	cout<<local_id<<current<<"->"<<dst_id<<destination<<"| ";

	for (int i=0; i<directions.size(); i++)
		cout<< directions[i]<<" ";
} // 

/*if (local_id == idfrom)// && dst_id==idto)
	cout<<" from: "<<dir_in<< " to: "<< dir_out<<" is: "<<in_directions<<endl; // */


bool in_directions=false;

for (int i=0; i<directions.size(); i++)
	if(dir_out==directions[i])
		in_directions=true;

return in_directions;

}



vector<int> DPNode::routingXYZ(const TCoord& current, const TCoord& destination)
{
  vector<int> directions;
  
  if (destination.x > current.x)
    directions.push_back(DIRECTION_EAST);
  else if (destination.x < current.x)
    directions.push_back(DIRECTION_WEST);
  else if (destination.y > current.y)
    directions.push_back(DIRECTION_SOUTH);
  else if (destination.y < current.y)
    directions.push_back(DIRECTION_NORTH);
  else if (destination.z < current.z)
    directions.push_back(DIRECTION_UP);
  else 
    directions.push_back(DIRECTION_DOWN);

  return directions;
}

vector<int> DPNode::routingOddEven1(const TCoord& current, 
				    const TCoord& source, const TCoord& destination)
{
  vector<int> directions;
  int c0 = current.x;
  int c1 = current.y;
  int s0 = source.x;
  //  int s1 = source.y;
  int d0 = destination.x;
  int d1 = destination.y;
  int e0, e1;

  e0 = d0 - c0;
  e1 = -(d1 - c1);

  if (e0 == 0)
    {
      if (e1 > 0)
	directions.push_back(DIRECTION_NORTH);
      else
	directions.push_back(DIRECTION_SOUTH);
    }
  else
   {
      if (e0 > 0)
	  {
	    if (e1 == 0)
		  {
		    directions.push_back(DIRECTION_EAST);
			if ((c0 % 2 == 0 || c0 == s0) && e0 != 1)			 // for NM routing  
			    {
					if (e1 > 0)
						directions.push_back(DIRECTION_NORTH);
					else
						directions.push_back(DIRECTION_SOUTH);			     
				}
		  }
	    
	    else
		{
			if ( (d0 % 2 == 1) && (e0 == 1) )
				{
				if (e1 > 0)
					directions.push_back(DIRECTION_NORTH);
				else
					directions.push_back(DIRECTION_SOUTH);
				}
			else
			{
				if ( (c0 % 2 == 0) || (c0 == s0) )
				{
				if (e1 > 0)
					directions.push_back(DIRECTION_NORTH);
				else
					directions.push_back(DIRECTION_SOUTH);
				}
	          	if ( (d0 % 2 == 0) || (e0 != 1) )
		     		 directions.push_back(DIRECTION_EAST);
			}
	    }
      }	// e0 >0
      else // e0<0
	  {
		directions.push_back(DIRECTION_WEST);
		if (c0 % 2 == 1)
		{
				if (e1 > 0)
					directions.push_back(DIRECTION_NORTH);
				else
					directions.push_back(DIRECTION_SOUTH);
			
		}
	  } // e0<0
  }// e0!= 0
  
  return directions;
} 
vector<int> DPNode::routingOddEven0(const TCoord& current, 
				    const TCoord& source, const TCoord& destination)
{
  vector<int> directions;

  int c0 = current.x;
  int c1 = current.y;
  int s0 = source.x;
  int s1 = source.y;
  int d0 = destination.x;
  int d1 = destination.y;
  int e0, e1;

  e0 = -(d0 - c0);
  e1 = d1 - c1;

  if (e1 == 0)
    {
       if (e0 > 0)
		directions.push_back(DIRECTION_WEST);
       if (e0 < 0)
		directions.push_back(DIRECTION_EAST);
    }
  else
     {
      if (e1 > 0)
	{
	  if (e0 == 0)
	    directions.push_back(DIRECTION_SOUTH);
	  else
	    {
	      if ( (c1 % 2 == 1) || (c1 == s1) )
		{
		if (e0 > 0)
			directions.push_back(DIRECTION_WEST);
	        if (e0 < 0)
			directions.push_back(DIRECTION_EAST);
		}
	      if ( (d1 % 2 == 1) || (e1 != 1) )
		directions.push_back(DIRECTION_SOUTH);
	    }
	}
      else
	{
	  directions.push_back(DIRECTION_NORTH);
	  if (c1 % 2 == 0)
	    {
	       if (e0 > 0)
			directions.push_back(DIRECTION_WEST);
	       if (e0 < 0)
			directions.push_back(DIRECTION_EAST);
	    }
	}
    }
  
  if (!(directions.size() > 0 && directions.size() <= 2))
  {
      cout << "\n STAMPACCHIO :";
      cout << source << endl;
      cout << destination << endl;
      cout << current << endl;

  }
  assert(directions.size() > 0 && directions.size() <= 2);
  
  return directions;
}




vector<int> DPNode::routingOddEvenNM(const TCoord& current, 
				    const TCoord& source, const TCoord& destination, const int dir_in)
{
  vector<int> directions;
  int c0 = current.x;
  int c1 = current.y;
  int s0 = source.x;
  //  int s1 = source.y;
  int d0 = destination.x;
  int d1 = destination.y;
  int e0, e1;

  e0 = d0 - c0;
  e1 = -(d1 - c1);

  if (e0 == 0)
    {
      if (e1 > 0)
	directions.push_back(DIRECTION_NORTH);
      else
	directions.push_back(DIRECTION_SOUTH);
    }
  else
   {
      if (e0 > 0)
	  {
	    if (e1 == 0)
		  {
		    directions.push_back(DIRECTION_EAST);
			if (((c0 % 2 == 1) || (c0 == s0)) && (e0 != 1))			 // for NM routing  
			     {
				if(c1 > 0 && dir_in != DIRECTION_NORTH ) 	
			              directions.push_back(DIRECTION_NORTH);
                   		if (c1 < TGlobalParams::mesh_dim_y-1 && dir_in != DIRECTION_SOUTH) 
				       directions.push_back(DIRECTION_SOUTH);	
			     }
		  }
	    
	    else
		{
			if ( (d0 % 2 == 0) && (e0 == 1) )
				{
				if (e1 > 0)
					directions.push_back(DIRECTION_NORTH);
				else
					directions.push_back(DIRECTION_SOUTH);
				}
			else
			{
				if ( (c0 % 2 == 1) || (c0 == s0) )
				{
					if(c1 > 0 && dir_in != DIRECTION_NORTH ) 	
						directions.push_back(DIRECTION_NORTH);
					if (c1 < TGlobalParams::mesh_dim_y-1 && dir_in != DIRECTION_SOUTH) 
						directions.push_back(DIRECTION_SOUTH);	
				}
	          	if ( (d0 % 2 == 1) || (e0 != 1) )
		     		 directions.push_back(DIRECTION_EAST);
			}
	    }
      }	// e0 >0
      else // e0<0
	  {
		directions.push_back(DIRECTION_WEST);
		if (c0 % 2 == 0)
		{
			if(c1 > 0 && dir_in != DIRECTION_NORTH ) 	
			          directions.push_back(DIRECTION_NORTH);
            		if(c1 < TGlobalParams::mesh_dim_y-1 && dir_in != DIRECTION_SOUTH) 
				      directions.push_back(DIRECTION_SOUTH);
		}
	  } // e0<0
  }// e0!= 0
  
  return directions;
} 

// odd even non mimumum the Odd Even rules are applied along the row and not the columns  <Nizar>
vector<int> DPNode::routingOddEvenNM0(const TCoord& current, 
				    const TCoord& source, const TCoord& destination, const int dir_in)
{
  vector<int> directions;

  int c0 = current.x;
  int c1 = current.y;
  int s0 = source.x;
  int s1 = source.y;
  int d0 = destination.x;
  int d1 = destination.y;
  int e0, e1;

  e0 = -(d0 - c0);
  e1 = d1 - c1;

  if (e1 == 0)
    {
       if (e0 > 0)
		directions.push_back(DIRECTION_WEST);
       if (e0 < 0)
		directions.push_back(DIRECTION_EAST);
    }
  else
     {
      if (e1 > 0)
	{
	  if (e0 == 0)
            {		 
	    directions.push_back(DIRECTION_SOUTH);
	    if ((c1 % 2 == 1 || c1 == s1) && e1 != 1)			 // for NM routing  
		{
			if(c0 > 0 && dir_in != DIRECTION_WEST) 	
			       directions.push_back(DIRECTION_WEST);
                	if (c0 < TGlobalParams::mesh_dim_x-1 && dir_in != DIRECTION_EAST) 
			       directions.push_back(DIRECTION_EAST);	
		}
	    }
	  else
	    {
	      if ( (d1 % 2 == 0) && (e1 == 1) )
				{
				if (e0 > 0)
					directions.push_back(DIRECTION_WEST);
	      			if (e0 < 0)
					directions.push_back(DIRECTION_EAST);
				}
			else
			{
				if ((c1 % 2 == 1) || (c1 == s1) )
				{
					if(c0 > 0 && dir_in != DIRECTION_WEST) 	
			       			directions.push_back(DIRECTION_WEST);
		                	if (c0 < TGlobalParams::mesh_dim_x-1 && dir_in != DIRECTION_EAST) 
					       directions.push_back(DIRECTION_EAST);	
				}
			      if ( (d1 % 2 == 1) || (e1 != 1) )
					directions.push_back(DIRECTION_SOUTH);
			}
	    }
	}
      else
	{
	  directions.push_back(DIRECTION_NORTH);
	  if (c1 % 2 == 0)
	    {
		if(c0 > 0 && dir_in != DIRECTION_WEST) 	
			directions.push_back(DIRECTION_WEST);
	      	if (c0 < TGlobalParams::mesh_dim_x-1 && dir_in != DIRECTION_EAST) 
	 	        directions.push_back(DIRECTION_EAST);	    
	    }
	}
    }

  return directions;
}


// odd even non mimumum the Odd Even rules are applied as Even-Odd rules <Nizar>
vector<int> DPNode::routingOddEvenNM1(const TCoord& current, 
				    const TCoord& source, const TCoord& destination, const int dir_in)
{
  vector<int> directions;
  int c0 = current.x;
  int c1 = current.y;
  int s0 = source.x;
  //  int s1 = source.y;
  int d0 = destination.x;
  int d1 = destination.y;
  int e0, e1;

  e0 = d0 - c0;
  e1 = -(d1 - c1);

  if (e0 == 0)
    {
      if (e1 > 0)
	directions.push_back(DIRECTION_NORTH);
      else
	directions.push_back(DIRECTION_SOUTH);
    }
  else
   {
      if (e0 > 0)
	  {
	    if (e1 == 0)
		  {
		    directions.push_back(DIRECTION_EAST);
			if ((c0 % 2 == 0 || c0 == s0) && e0 != 1)			 // for NM routing  
			     {
				if(c1 > 0 && dir_in != DIRECTION_NORTH ) 	
			              directions.push_back(DIRECTION_NORTH);
                   		if (c1 < TGlobalParams::mesh_dim_y-1 && dir_in != DIRECTION_SOUTH) 
				       directions.push_back(DIRECTION_SOUTH);	
			     }
		  }
	    
	    else
		{
			if ( (d0 % 2 == 1) && (e0 == 1) )
				{
				if (e1 > 0)
					directions.push_back(DIRECTION_NORTH);
				else
					directions.push_back(DIRECTION_SOUTH);
				}
			else
			{
				if ( (c0 % 2 == 0) || (c0 == s0) )
				{
					if(c1 > 0 && dir_in != DIRECTION_NORTH ) 	
						directions.push_back(DIRECTION_NORTH);
					if (c1 < TGlobalParams::mesh_dim_y-1 && dir_in != DIRECTION_SOUTH) 
						directions.push_back(DIRECTION_SOUTH);	
				}
	          	if ( (d0 % 2 == 0) || (e0 != 1) )
		     		 directions.push_back(DIRECTION_EAST);
			}
	    }
      }	// e0 >0
      else // e0<0
	  {
		directions.push_back(DIRECTION_WEST);
		if (c0 % 2 == 1)
		{
			if(c1 > 0 && dir_in != DIRECTION_NORTH ) 	
			          directions.push_back(DIRECTION_NORTH);
            		if(c1 < TGlobalParams::mesh_dim_y-1 && dir_in != DIRECTION_SOUTH) 
				  directions.push_back(DIRECTION_SOUTH);
		}
	  } // e0<0
  }// e0!= 0
  
  return directions;
} 

//Vertically dw- Horizontally Odd Even <Nizar>
bool DPNode::can_turnDwOddEven(int dir_in, int dir_out, int dst_id)
{
  TCoord current  	= id2Coord(local_id);
  TCoord destination 	= id2Coord(dst_id);
  TCoord source 	= current;
  //if (dir_in==dir_out)
  //         return false;

  switch ( dir_in ) {

  case DIRECTION_NORTH : 
	     source.y-=1;
	     break;
  case DIRECTION_EAST : 
	    source.x+=1;
	    break;
  case DIRECTION_SOUTH : 
	    source.y+=1;
	    break;
  case DIRECTION_WEST: 
	    source.x-=1;
	    break;
  case DIRECTION_UP : 
	    source.z+=1;
	    break;
  case DIRECTION_DOWN : 
	    source.z-=1;
	    break;
  default:
	// you must not be here !
	assert (false);
}

  vector<int> directions;

  int cx = current.x;
  int cy = current.y;
  int cz = current.z;
  
  int sx = source.x;
  int sy = source.y;
  int sz = source.z;
  
  int dx = destination.x;
  int dy = destination.y;
  int dz = destination.z;
 
  //int dirz=cz-sz;   // to check if a packet is in a nonminimum z route
  //bool dwrange= true; //(dirz >=0 && dirz < 1);

   //int e0, e1, e2;
 

if (dz > cz )   // packet is moving DOWN
	directions.push_back(DIRECTION_DOWN);

if (dz < cz)  // UP
	{
	if ((dx==cx) & (dy==cy))
		directions.push_back(DIRECTION_UP);
		else 
		{
 			directions=routingOddEvenNM(current, source, destination, dir_in);

	
      		if (cz < TGlobalParams::mesh_dim_z-1)
	    		 directions.push_back(DIRECTION_DOWN); 	
		}

	}  // end moving UP
  
  if (cz == dz)  // co-palanr
     {
		directions=routingOddEvenNM(current, source, destination, dir_in);

     if (cz < TGlobalParams::mesh_dim_z-1 )
     		directions.push_back(DIRECTION_DOWN); 	
     }



 bool in_directions=false;

for (int i=0; i<directions.size(); i++)
	if(dir_out==directions[i])
		in_directions=true;

return in_directions;

}


bool DPNode::can_turnOddEvenNM(int dir_in, int dir_out, int dst_id)
{
  TCoord current  	= id2Coord(local_id);
  TCoord destination 	= id2Coord(dst_id);
  TCoord source 	= current;

  switch ( dir_in ) {

  case DIRECTION_NORTH : 
	     source.y-=1;
	     break;
  case DIRECTION_EAST : 
	    source.x+=1;
	    break;
  case DIRECTION_SOUTH : 
	    source.y+=1;
	    break;
  case DIRECTION_WEST: 
	    source.x-=1;
	    break;
  case DIRECTION_UP : 
	    source.z+=1;
	    break;
  case DIRECTION_DOWN : 
	    source.z-=1;
	    break;
  default:
	// you must not be here !
	assert (false);
}
   vector<int> directions;
   int sz=source.z;
   int cz=current.z;
   int dz=destination.z;
   
   //int sx=source.x;
   int cx=current.x;
   int dx=destination.x;
   
   //int sy=source.y;
   int cy=current.y;
   int dy=destination.y;

   int ex = dx-cx;
   int ey = dy-cy;
   int ez = dz-cz;
   int dirz=cz-sz; 
   bool dwrange= true; //(dirz >=0 && dirz <1);

   if (ez == 0)
	{
	if (cz%2==0) // for even z use the modified OE routing
		directions=routingOddEvenNM(current, source, destination, dir_in);
	else // for odd use convensional OE routing
		directions=routingOddEvenNM(current, source, destination, dir_in);
	// to move down: cz<dimz AND no reflection AND [either continuing to down OR this is even Z]	
	if (cz < TGlobalParams::mesh_dim_z-1 && dir_in !=DIRECTION_DOWN)// && cz % 2 == 0 && dwrange )
    		directions.push_back(DIRECTION_DOWN); 
	}
  else
    {
	if (ez < 0)   // z direction is -ve
	{
	    if ((ex==0) && (ey == 0))  // on the xy position 
			directions.push_back(DIRECTION_UP);
	    else
	    {
	      //if ( cz % 2 == 1 || cz == sz || dirz > 0)
			if (cz%2==0) 
				directions=routingOddEvenNM(current, source, destination, dir_in);
			else 
				directions=routingOddEvenNM(current, source, destination, dir_in);

	      //if ( (dz % 2 == 1 || ez != -1)  && dir_in !=DIRECTION_UP && dirz < 0)
	      //		directions.push_back(DIRECTION_UP);

   	      if ((cz < TGlobalParams::mesh_dim_z-1) && dir_in !=DIRECTION_DOWN) //&&  cz % 2 == 0 && dwrange)
    	      		directions.push_back(DIRECTION_DOWN); 

	    }
	}
	  else   // ez > 0		
	  {
	        if ((ex!=0 || ey!=0))//&&  (cz % 2 == 0))  // need xy-plane routing and the z plane is even
		{
			if (cz%2==0) 
				directions=routingOddEvenNM(current, source, destination, dir_in);
			else 
				directions=routingOddEvenNM(current, source, destination, dir_in);
		}
		
		 directions.push_back(DIRECTION_DOWN);
	  }
    }


bool in_directions=false;

for (int i=0; i<directions.size(); i++)
	if(dir_out==directions[i])
		in_directions=true;

return in_directions;
}


// Balanced minimum odd-even <Nizar>
bool DPNode::can_turnOddEvenBalanced(int dir_in, int dir_out, int dst_id)
{
  TCoord current = id2Coord(local_id);
  TCoord dest    = id2Coord(dst_id);

  // Straight-through and 180° are always allowed (or handle U-turns per your policy)
  if (dir_in == dir_out) return true;

  // Determine plane orientation: even plane row-wise (OE0), odd plane column-wise (OE1)
  // For OE0 (row-wise, uses x as the "column" axis):
  int col = current.x;   // parity axis for row-wise odd-even

  // Odd-even turn restrictions (Chiu), row-wise variant:
  // Forbidden in EVEN column: any turn FROM East to North or South
  if (col % 2 == 0) {
    if (dir_in == DIRECTION_EAST && (dir_out == DIRECTION_NORTH || dir_out == DIRECTION_SOUTH))
      return false;
  }
  // Forbidden in ODD column: any turn TO West from North or South
  if (col % 2 == 1) {
    if ((dir_in == DIRECTION_NORTH || dir_in == DIRECTION_SOUTH) && dir_out == DIRECTION_WEST)
      return false;
  }

  // Vertical (z) turns: allow only toward destination plane per your minimal rule
  // (mirror routingOddEvenBalanced's ez gating)
  int ez = dest.z - current.z;
  if (dir_out == DIRECTION_UP   && ez >= 0) return false;   // don't go up if dst is same/below
  if (dir_out == DIRECTION_DOWN && ez <= 0) return false;   // don't go down if dst is same/above

  return true;   // turn permitted
}
