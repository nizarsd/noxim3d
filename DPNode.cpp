#include "DPNode.h"
#include <algorithm>
#define alpha 1

void DPNode::dpProcess()
{
	// Gating for other selection methods
	if (TGlobalParams::selection_strategy != SEL_DP)  return;

	int stime  = (int) (sc_time_stamp().to_double()/1000 - DEFAULT_RESET_TIME);
	int no_dst = TGlobalParams::mesh_dim_x*TGlobalParams::mesh_dim_y*TGlobalParams::mesh_dim_z;

	// dwell = 2: phase 0 drives stored costs, phase 1 relaxes coherently
	int phase = stime % 2;
	dst_id    = (stime / 2) % no_dst;

	if (reset.read())
	{
		for (int i=0; i<DIRECTIONS; i++)
		{
			dp_tx[i].write  (BIG_VALUE);
			dp_dir[i].write (NOT_VALID);
		}
		for (int d=0; d<DPSIZE; d++)
			for (int i=0; i<DIRECTIONS; i++)
				cost_mem[d][i] = BIG_VALUE;
		return;
	}

	if (!dp_clock.posedge())  return;

	if (local_id == dst_id)          // destination node: cost anchor
	{
		for (int i=0; i<DIRECTIONS; i++)
		{
			dp_tx[i].write (0);      // both phases: advertise zero
			dp_dir[i].write(NOT_VALID);
			cost_mem[dst_id][i] = 0;
		}
		return;
	}

	if (phase == 0)                  // DRIVE: put stored costs for dst_id on wires
	{
		for (int i=0; i<DIRECTIONS; i++)
			dp_tx[i].write (cost_mem[dst_id][i]);
		// dp_dir untouched — router latches only on phase 1
		return;
	}

	// phase 1 — RELAX: dp_rx coherently holds neighbours' stored costs for dst_id
	int temp_dp [DIRECTIONS];
	for (int i=0; i<DIRECTIONS; i++)
		temp_dp[i] = (int)(dp_rx[i]*alpha) + local_dp_cost;

	int sorted_ports[] = {0,1,2,3,4,5};
	BubbleSort(temp_dp, sorted_ports);          // sort ports by their cost

	int HIGH_COST = BIG_VALUE;
	int dp_cost [DIRECTIONS] = {HIGH_COST,HIGH_COST,HIGH_COST,HIGH_COST,HIGH_COST,HIGH_COST};

	for (int i=0; i<DIRECTIONS; i++)            // min cost to directions with legal turns
		for (int j=0; j<DIRECTIONS; j++)
			if (can_turn(j, sorted_ports[i], dst_id) && dp_cost[j] > temp_dp[sorted_ports[i]])
				dp_cost[j] = temp_dp[sorted_ports[i]];

	for (int i=0; i<DIRECTIONS; i++)
	{
		dp_tx[i].write (dp_cost[i]);
		dp_dir[i].write(sorted_ports[i]);
		cost_mem[dst_id][i] = dp_cost[i];       // persist for next round
	}
}


// void DPNode::dpProcess()
// {
	// // Gating for other selection methods
	// if (TGlobalParams::selection_strategy != SEL_DP)  return; 
	
		
		// //int dp_time=2;
		// int stime   = (int) (sc_time_stamp().to_double()/1000 - DEFAULT_RESET_TIME);
		// // int cFlag = (stime%dp_time) ;//&& (stime>=0);
		// // number of desinations (total no of nodes)
		// int no_dst=TGlobalParams::mesh_dim_x*TGlobalParams::mesh_dim_y*TGlobalParams::mesh_dim_z; 
		// int dp_dwell= 1; //(TGlobalParams::mesh_dim_x + TGlobalParams::mesh_dim_y + TGlobalParams::mesh_dim_z);  // >= mesh diameter (dx-1 + dy-1 + dz-1)
		// int dst_id = (stime / dp_dwell) % no_dst;

		// int	 dp_cost [DIRECTIONS];
		// int idfrom=99, idto=0;
		// //          if (local_id==10 && dst_id<no_dst)
		// //	           cout<<stime<<":"<<dst_id<<endl;

		// /*//   dst_id = (stime%no_dst);     ///<Nizar>
		// if (dst_id==idto && local_id==idfrom)
		// {
			// cout<<" stime: "<< stime<< " local_id: "<<local_id <<endl; 
			// for(int i=0; i<DIRECTIONS; i++)
			// cout<< dp_rx[i]<<"  ";

			// cout<< endl;		
		// }// */
       

	  // if (reset.read())
	   // {
             // for(int i=0; i<DIRECTIONS; i++) 
		 // {
		   // dp_tx[i].write   (BIG_VALUE);
		   // dp_dir[i].write  (NOT_VALID);
		 // }
		// }
	 // else if (dp_clock.posedge())
	  // {
	   // if (local_id ==  dst_id && dst_id < no_dst )  // if the current node is the destination node
		// {    
		
		  // for(int i=0; i<DIRECTIONS; i++)
			// {
			 // dp_tx[i].write (0); 
			 // dp_dir[i].write(NOT_VALID);
			// }
		// }
	 
	 // else if (local_id != dst_id  && dst_id < no_dst)
		// {
		 // int temp_dp [DIRECTIONS]; 
		 // int min      = BIG_VALUE;
		 // int best_dir = NOT_VALID;
		 // int best_dir2 =NOT_VALID;
        
 // /*               if (local_id == idfrom && dst_id==idto) 
			 // for(int j=0; j<DIRECTIONS; j++)  
				 // for(int i=0; i<DIRECTIONS; i++)  
					// cout<<" stime: "<< stime<<" can turn "<<i << " -> "<< j <<" from"<<local_id<< " to " <<dst_id<< " is " <<  can_turn(i,j, dst_id)<<endl;  // */

              	 // for(int i=0; i<DIRECTIONS; i++)   // find the minimum cost
					// temp_dp[i] = (int)(dp_rx[i]*alpha) + local_dp_cost; //used_buffer_size[i];

		
		// int sorted_ports[]={0,1,2,3,4,5};
  		// BubbleSort(temp_dp, sorted_ports);  // sort ports by thier cost

		// int HIGH_COST=BIG_VALUE;  // cost to not possible turn directions 
		// int dp_cost [DIRECTIONS]={HIGH_COST,HIGH_COST,HIGH_COST,HIGH_COST,HIGH_COST,HIGH_COST};     // the cost that will be propagated to each direction
		
		 // for(int i=0; i<DIRECTIONS; i++)  // send the min cost to directions with possible turns to current dst_id
			// for (int j=0; j<DIRECTIONS; j++)
			// {
				// if (can_turn(j,sorted_ports[i], dst_id) && dp_cost[j] > temp_dp[sorted_ports[i]])   // this function is not dependent on dir_in !!
				// dp_cost[j]=temp_dp[sorted_ports[i]];
			// }
			
                 

        


			// for(int i=0; i<DIRECTIONS; i++)
			// {
				// dp_tx[i].write (dp_cost [i]);   	
				// dp_dir[i].write(sorted_ports[i]);
			// }

				 

		 
	// } // local id <> dest id
   // } // dp_clock.posedge() && dst_id < no_dst
// } // process end 

//---------------------------------------------------------------------------


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
					directions.push_back(DIRECTION_SOUTH);			     }
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
// MUST mirror TRouter::routingOddEvenBalanced exactly — change both together.
bool DPNode::can_turnOddEvenBalanced(int dir_in, int dir_out, int dst_id)
{
  TCoord current     = id2Coord(local_id);
  TCoord destination = id2Coord(dst_id);
  TCoord source      = current;

  switch (dir_in) {              // reconstruct one-hop-back source
  case DIRECTION_NORTH: source.y -= 1; break;
  case DIRECTION_EAST:  source.x += 1; break;
  case DIRECTION_SOUTH: source.y += 1; break;
  case DIRECTION_WEST:  source.x -= 1; break;
  case DIRECTION_UP:    source.z += 1; break;
  case DIRECTION_DOWN:  source.z -= 1; break;
  default: assert(false);
  }

  vector<int> directions;

  int sz = source.z,      cz = current.z,  dz = destination.z;
  int ex = destination.x - current.x;
  int ey = destination.y - current.y;
  int ez = dz - cz;
  
    if (ez == 0)
  {
    // on destination plane: parity-split in-plane only
    if (cz % 2 == 0)
      directions = routingOddEven0(current, source, destination); // row-wise
    else
      directions = routingOddEven1(current, source, destination); // column-wise
  }
  else if (ez > 0)   // going down
  {
    if ((ex == 0) && (ey == 0))
      directions.push_back(DIRECTION_DOWN);      // aligned: descend only
    else
    {
      if ((cz % 2 == 1) || (cz == sz))           // in-plane on odd or source plane
      {
        if (cz % 2 == 0)
          directions = routingOddEven0(current, source, destination);
        else
          directions = routingOddEven1(current, source, destination);
      }
      if ((dz % 2 == 1) || (ez != 1))            // descend under published condition
        directions.push_back(DIRECTION_DOWN);
    }
  }
  else               // ez < 0, going up
  {
    // exclusivity preserved from the published algorithm:
    // unaligned + even plane -> in-plane ONLY; otherwise UP only
    if ((ex != 0 || ey != 0) && (cz % 2 == 0))
      directions = routingOddEven0(current, source, destination); // even plane: row-wise
    else
      directions.push_back(DIRECTION_UP);
  }
  
  for (unsigned int i = 0; i < directions.size(); i++)
    if (dir_out == directions[i])
      return true;
  return false;
}