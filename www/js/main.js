
var board_size;
var komi;
var moves;
var last_move;
var next_color;
var simple_ko;
var engine_color='none';
var passes;
var threads;
var playouts;
var time;
var black_captures;
var white_captures;
var game_over=false;
var paper=null;
var size_chg=true;
var thinking=false;
var board_data=null;
var board_scored=null;
var spinners=null;
var spinnercount=12;
var spinnerp=2000;
var spinneri=0;
var spinnertick=null;
var buttons={};
var buttons_enabled={};
var info='';
var gfx_cache=null;
var justundid=false;

var header='m 0,0 c -12.737776,0 -23.0625,10.32472 -23.0625,23.0625 0,12.73778 10.324724,23.0625 23.0625,23.0625 12.737776,0 23.0625,-10.32472 23.0625,-23.0625 0,-12.73778 -10.324724,-23.0625 -23.0625,-23.0625 z m 95.34375,5.25 0,35 3.5,0 0,-5.375 4.09375,-5 8.625,10.375 4.34375,0 -10.75,-12.90625 10.09375,-13 -4.3125,0 -12.09375,15.71875 0,-24.8125 -3.5,0 z m 35.53125,0 c -6.67731,0 -10.03221,3.07943 -9.9375,9.09375 l -4.03125,0 0,3.15625 4.03125,0 0,22.75 3.5,0 0,-22.75 6.4375,0 0,-3.15625 -6.4375,0 c -0.0474,-4.30947 1.65446,-5.89014 6.4375,-5.9375 l 0,-3.15625 z m -135.15625,2.28125 c 0.749161,0.0219 1.50284,0.25813 2.0625,0.65625 0.994745,0.70764 1.571668,2.23049 1.25,3.40625 -0.128049,0.46805 -0.528941,0.90374 -1,1.125 l 4.8125,0 c -0.400303,-0.16571 -0.741007,-0.44313 -0.875,-0.8125 -0.461401,-1.2719 0.256786,-3.0787 1.375,-3.84375 1.133744,-0.77566 2.974435,-0.67124 4.09375,0.125 0.994745,0.70764 1.602918,2.23049 1.28125,3.40625 -0.12805,0.46805 -0.528941,0.90374 -1,1.125 l 9.28125,0 0,6.90625 -1.75,0 c -1.014379,5.77752 -2.667923,5.04419 -3.46875,0 l -22.71875,0 c -0.785049,4.25526 -2.533098,6.10404 -3.53125,0 l -1.9375,0 0,-0.8125 c -0.3378,0.10372 -2.115822,0.69342 -2.40625,1.5625 -0.471376,1.41055 0.450321,3.20524 1.59375,4.15625 1.795296,1.49318 4.540158,1.31555 6.875,1.28125 3.277273,-0.0482 6.480982,-3.02003 9.625,-2.09375 2.219187,0.6538 3.901068,2.85379 4.90625,4.9375 0.941652,1.95201 -0.124334,4.79384 0.71875,6.46875 1.032232,2.05068 1.34375,4.4375 1.34375,4.4375 l -1.34375,0 c 0.14198,0.7613 0.0125,2.623 -0.71875,2.625 -0.827517,0.002 -0.75,-2.75 -0.75,-2.75 l -1.5625,0 c -0.05206,1.63371 0.09229,2.7333 -0.84375,2.6875 -0.912477,-0.0446 -0.5625,-2.6875 -0.5625,-2.6875 l -1.15625,-0.125 c 0.18608,-2.63357 1.064465,-3.11081 1.875,-4.625 0.859797,-1.60623 1.592292,-3.60061 1.0625,-5.34375 -0.47433,-1.56064 -1.906735,-2.99895 -3.46875,-3.46875 -2.892219,-0.86987 -6.057869,1.82165 -8.875,1.84375 -3.162012,0.0248 -6.334432,0.19213 -8.5,-1.71875 -1.52678,-1.34722 -2.420704,-3.9968 -1.90625,-5.78125 0.515975,-1.78972 3.840351,-2.567 4.09375,-2.625 l 0,-4.875 9.5625,0 c -0.400303,-0.16571 -0.772257,-0.44313 -0.90625,-0.8125 -0.4614,-1.2719 0.288036,-3.0787 1.40625,-3.84375 0.566872,-0.38783 1.31334,-0.55315 2.0625,-0.53125 z m 229.9375,6.15625 c -4.02533,0 -6.82159,1.54295 -9,5 -2.03634,-3.45705 -4.61674,-4.96875 -8.5,-4.96875 -3.26762,0 -5.29226,0.98913 -7.28125,3.59375 l 0,-2.96875 -3.5,0 0,25.90625 3.5,0 0,-15.125 c 0,-5.16189 2.61192,-8.1875 6.96875,-8.1875 2.13106,0 4.12762,0.85958 5.40625,2.375 1.13656,1.32599 1.59375,3.11316 1.59375,5.8125 l 0,15.125 3.5,0 0,-15.03125 c 0,-2.50991 0.39758,-4.11247 1.25,-5.34375 1.23128,-1.89427 3.41245,-2.96875 5.875,-2.96875 2.27313,0 4.25358,0.87473 5.4375,2.4375 0.94714,1.32599 1.40625,3.27038 1.40625,5.875 l 0,15.03125 3.5,0 0,-15.40625 c 0,-3.40969 -0.59444,-5.62183 -2.0625,-7.46875 -1.79956,-2.36784 -4.73142,-3.6875 -8.09375,-3.6875 z m -181.0625,0.0312 c -7.6718,0 -13.59375,5.82916 -13.59375,13.40625 0,7.95594 5.82434,13.71875 13.875,13.71875 7.57709,0 13.15625,-5.73348 13.15625,-13.5 0,-7.90859 -5.67098,-13.625 -13.4375,-13.625 z m 31.03125,0 c -7.76652,0 -13.625,5.85848 -13.625,13.625 0,7.71916 5.91906,13.5 13.875,13.5 4.40418,0 7.22205,-1.46242 9.96875,-5.15625 l 0,4.5625 3.5,0 0,-25.90625 -3.5,0 0,4.59375 c -2.55727,-3.59912 -5.71985,-5.21875 -10.21875,-5.21875 z m 71,0 c -7.6718,0 -13.59375,5.82916 -13.59375,13.40625 0,7.95594 5.82434,13.71875 13.875,13.71875 7.57709,0 13.15625,-5.73348 13.15625,-13.5 0,-7.90859 -5.67098,-13.625 -13.4375,-13.625 z m 31.03125,0 c -7.76652,0 -13.65625,5.85848 -13.65625,13.625 0,7.71916 5.91906,13.5 13.875,13.5 4.40418,0 7.2533,-1.46242 10,-5.15625 l 0,4.5625 3.5,0 0,-25.90625 -3.5,0 0,4.59375 c -2.55727,-3.59912 -5.71985,-5.21875 -10.21875,-5.21875 z m -133.0625,3.21875 c 5.68282,0 9.90625,4.45443 9.90625,10.46875 0,5.9196 -4.08329,10.21875 -9.71875,10.21875 -5.96696,0 -10.21875,-4.34458 -10.21875,-10.40625 0,-5.87224 4.30108,-10.28125 10.03125,-10.28125 z m 31.125,0 c 5.68282,0 10.125,4.44122 10.125,10.21875 0,5.87224 -4.3494,10.46875 -9.9375,10.46875 -5.87224,0 -10.375,-4.5804 -10.375,-10.5 0,-5.63546 4.55204,-10.1875 10.1875,-10.1875 z m 70.90625,0 c 5.68282,0 9.875,4.45443 9.875,10.46875 0,5.9196 -4.05204,10.21875 -9.6875,10.21875 -5.96696,0 -10.25,-4.34458 -10.25,-10.40625 0,-5.87224 4.33233,-10.28125 10.0625,-10.28125 z m 31.125,0 c 5.68282,0 10.125,4.44122 10.125,10.21875 0,5.87224 -4.3494,10.46875 -9.9375,10.46875 -5.87224,0 -10.375,-4.5804 -10.375,-10.5 0,-5.63546 4.55204,-10.1875 10.1875,-10.1875 z';
var icons=
{
  'new':'m 16,0 c -8.91111,0 -16.14318,7.19599 -16.14318,16.06266 0,8.86666 7.23207,16.0528 16.14318,16.0528 8.91111,0 16.13327,-7.18614 16.13327,-16.0528 0,-8.86667 -7.22216,-16.06266 -16.13327,-16.06266 z m -11.68647,12.24901 c 0,0 22.99624,0.046 23.36304,0 l 0,4.79909 -1.22807,0 c -0.70964,4.02168 -1.86619,3.51122 -2.42643,0 l -15.89558,0 c -0.54921,2.96205 -1.76776,4.24897 -2.46605,0 l -1.34691,0 0,-4.79909 z',
  'pass':'m 16,0 c -8.91111,0 -16.13327,7.18613 -16.13327,16.0528 0,8.86666 7.22216,16.0528 16.13327,16.0528 8.91111,0 16.13327,-7.18614 16.13327,-16.0528 0,-8.86667 -7.22216,-16.0528 -16.13327,-16.0528 z m 3.43661,5.42977 5.04103,7.27254 -3.11969,0.0394 c 0,0 0.3081,7.58881 -1.45586,10.77085 -1.63262,2.30405 -4.48315,3.51841 -6.99207,3.07457 -2.19445,-0.3882 -3.82631,-2.37537 -4.81325,-4.62171 -1.35955,-3.09444 0.13866,-11.07633 0.13866,-11.07633 0.90787,6.31546 0.55563,11.42276 5.55602,12.87969 1.45574,0.29182 3.05855,-0.99445 3.87239,-2.29608 1.51505,-2.42312 0.87153,-8.65215 0.87153,-8.65215 l -3.51585,0.0394 4.41709,-7.43021 z',
  'genmove':'m 16,0 c -8.91112,0 -16.13328,7.19599 -16.13328,16.06266 0,8.86666 7.22216,16.0528 16.13328,16.0528 8.91111,0 16.13327,-7.18614 16.13327,-16.0528 0,-8.86667 -7.22216,-16.06266 -16.13327,-16.06266 z m -6.17997,8.65215 12.35993,7.71599 -12.35993,7.10502 0,-14.82101 z',
  'stop':'m 16,0 c -8.91112,0 -16.13328,7.18614 -16.13328,16.0528 0,8.86667 7.22216,16.05281 16.13328,16.05281 8.91111,0 16.13327,-7.18614 16.13327,-16.05281 0,-8.86666 -7.22216,-16.0528 -16.13327,-16.0528 z m -6.49689,9.58832 12.99377,0 0,12.92896 -12.99377,0 0,-12.92896 z',
  'info':'m 16,0 c -8.91111,0 -16.13327,7.19599 -16.13327,16.06266 0,8.86666 7.22216,16.0528 16.13327,16.0528 8.91111,0 16.14318,-7.18614 16.14318,-16.0528 0,-8.86667 -7.23207,-16.06266 -16.14318,-16.06266 z m -0.48529,7.6273 c 0.0527,-0.005 0.11427,0 0.16837,0 0.86561,0 1.5648,0.67929 1.5648,1.52743 0,0.84814 -0.69919,1.53728 -1.5648,1.53728 -0.86561,0 -1.5747,-0.68914 -1.5747,-1.53728 0,-0.79513 0.616,-1.44879 1.40633,-1.52743 z m 0.86163,5.04545 c 0,0 -2.92057,7.11696 -0.8022,9.48977 1.0044,1.12505 4.53593,0.14782 4.53593,0.14782 0,0 -2.86148,2.29311 -4.53593,2.17782 -1.29105,-0.0889 -2.67487,-0.97457 -3.22864,-2.1384 -1.38901,-2.91924 0.95077,-9.64745 0.95077,-9.64745 l 3.08007,-0.0295 z',
  'undo':'m 16,0 c -8.91112,0 -16.13328,7.18613 -16.13328,16.0528 0,8.86666 7.22216,16.0528 16.13328,16.0528 8.91111,0 16.13327,-7.18614 16.13327,-16.0528 0,-8.86667 -7.22216,-16.0528 -16.13327,-16.0528 z m -3.05037,9.12516 0.0792,3.3505 c 0,0 6.50604,-0.007 9.27985,1.8132 2.57657,2.49114 3.14513,4.74633 3.29796,8.69158 0,0 -2.31247,-4.50794 -4.65478,-5.72541 -2.34231,-1.21746 -7.8438,-1.75408 -7.8438,-1.75408 l -0.16837,3.34064 -6.5464,-4.93705 6.55631,-4.77938 z',
  'help':'m 16,0 c -8.91111,0 -16.14318,7.18614 -16.14318,16.0528 0,8.86667 7.23207,16.05281 16.14318,16.05281 8.91111,0 16.13327,-7.18614 16.13327,-16.05281 0,-8.86666 -7.22216,-16.0528 -16.13327,-16.0528 z m -0.5249,5.77467 c 1.89578,-0.0215 4.94274,1.3741 6.12054,2.94647 0.60549,1.79493 1.06743,3.81339 0.23769,5.89292 -0.57946,1.45227 -3.51226,2.09558 -4.38738,3.82351 -0.44186,0.87245 -0.24759,2.92675 -0.24759,2.92675 l -2.39672,-0.0591 c 0,0 -0.2031,-3.44806 0.64375,-4.8385 0.75526,-1.24006 2.83207,-1.25576 3.57527,-2.50302 0.61087,-1.02517 0.98376,-2.50354 0.40605,-3.54758 -0.69012,-1.2472 -2.43988,-2.01549 -3.86248,-1.86248 -1.82449,0.19625 -4.32795,3.38991 -4.32795,3.38991 l -1.52519,-1.76393 c 0,0 2.98965,-3.62927 5.03113,-4.30637 0.21473,-0.0649 0.46205,-0.0955 0.73288,-0.0986 z m 0.68336,17.10723 c 0.95992,0 1.73316,0.77924 1.73316,1.73437 0,0.95513 -0.77324,1.72452 -1.73316,1.72452 -0.95992,0 -1.74307,-0.76939 -1.74307,-1.72452 0,-0.95513 0.78315,-1.73437 1.74307,-1.73437 z',
  'settings':'m 16,0 c -8.91112,0 -16.13328,7.196 -16.13328,16.06266 0,8.86667 7.22216,16.0528 16.13328,16.0528 8.91111,0 16.14317,-7.18613 16.14317,-16.0528 0,-8.86666 -7.23206,-16.06266 -16.14317,-16.06266 z m 0.36644,6.13929 c 1.07836,-0.0174 2.72354,0.96573 2.72354,0.96573 l -2.73345,3.57714 1.06961,2.19753 2.55518,-0.18723 2.56508,-3.73481 c 0,0 2.43516,1.6063 2.00057,2.77893 -0.64217,1.7327 -1.73591,3.70831 -2.57499,4.41477 -0.78004,0.65676 -2.3901,-0.38142 -3.11969,0.3252 -1.04525,1.01232 -3.2881,4.10772 -6.11064,8.52404 -0.55379,0.8665 -2.80653,1.33209 -3.89219,0.63069 -1.14691,-0.74098 -2.11656,-2.00279 -0.83192,-3.85307 3.31074,-5.01204 4.93748,-6.38459 5.29853,-7.91308 0.2347,-0.99358 -0.7521,-1.64847 -0.74278,-2.5917 0.0159,-1.60819 0.70776,-2.90642 3.1593,-4.93705 0.16189,-0.1341 0.38499,-0.19308 0.63385,-0.19709 z',
};

function xy2pos(x,y)
{
  if (x>7) x++; // there is no I column
  return String.fromCharCode('A'.charCodeAt()+x)+(y+1);
}

function doGtpCmdThink(cmd,args,func)
{
  if (!thinking)
  {
    thinking=true;
    spinnersStart();
  }
  drawBoard();
  disableButton('new');
  disableButton('pass');
  disableButton('genmove');
  disableButton('undo');
  disableButton('settings');
  enableButton('stop');
  $('#status').html('Engine is thinking...');
  doGtpCmd(cmd,args,func)
}

function doGtpCmd(cmd,args,func)
{
  $.get(cmd+'.gtpcmd?'+args,func);
}

function isHoshi(x,y)
{
  size=board_size;
  dx=x;
  if (dx>(size/2))
    dx=size-dx-1;
  dy=y;
  if (dy>(size/2))
    dy=size-dy-1;
  if (dx>dy)
  {
    t=dx;
    dx=dy;
    dy=t;
  }

  if (size==9)
  {
    if (dx==2 && dy==2)
      return true;
    else if (dx==4 && dy==4)
      return true;
    else
      return false;
  }
  else if (size==13)
  {
    if (dx==3 && dy==3)
      return true;
    else if (dx==6 && dy==6)
      return true;
    else
      return false;
  }
  else if (size==19)
  {
    if (dx==3 && dy==3)
      return true;
    else if (dx==9 && dy==9)
      return true;
    else if (dx==3 && dy==9)
      return true;
    else
      return false;
  }
  else
    return false;
}

function spinnersStart()
{
  (function ticker() {
    if (!thinking)
      return;
    n=spinnercount;
    for (i=0;i<n;i++) {
      j=(spinneri+i)%n;
      spinners[j].attr('opacity',i/n);
    }
    spinneri=(spinneri+1)%n;
    spinnertick=setTimeout(ticker,spinnerp/n);
  })();
}

function drawBoard()
{
  var data=board_data;
  var sz=board_size*24;
  var lw=1.5;
  var crad=11;
  var bdr=4;
  if (size_chg)
  {
    if (paper!=null)
    {
      paper.remove();
      for (i=0;i<n;i++)
      {
        spinners[i].remove();
      }
    }
    width=board_size*24+45+2*bdr;
    $('#page').width(width); // dynamic size
    $('#status').width(width-5);
    paper=Raphael('board',sz+2*bdr,sz+2*bdr);

    n=spinnercount;
    csz=sz/(n*2.5);
    cxy=sz/2+bdr;
    spinners=[];
    for (i=0;i<spinnercount;i++)
    {
      alpha=i*Math.PI*2/n;
      x=0.15*sz*Math.cos(alpha);
      y=0.15*sz*Math.sin(alpha);
      spinners[i]=paper.circle(cxy+x,cxy+y,csz).attr({fill:'#fff',stroke:'none',opacity:0});
    }

    spinnersStart();
    size_chg=false;
  }

  if (gfx_cache!=null)
  {
    gfx_cache.remove();
  }
  gfx_cache=paper.set();

  gfx_cache.push(paper.rect(1,1,sz-2+2*bdr,sz-2+2*bdr).attr({fill:'#c8a567', stroke:'#352b17', 'stroke-width':1}));
  for (i=0;i<board_size;i++)
  {
    gfx_cache.push(paper.rect(i*24+12-lw/2+bdr,12-lw/2+bdr,lw,sz-24+lw).attr({fill:'#000000', stroke:'none'}));
    gfx_cache.push(paper.rect(12-lw/2+bdr,i*24+12-lw/2+bdr,sz-24+lw,lw).attr({fill:'#000000', stroke:'none'}));
  }
  for (x=0;x<board_size;x++)
  {
    for (y=0;y<board_size;y++)
    {
      if (isHoshi(x,y))
        gfx_cache.push(paper.circle(x*24+12+bdr,y*24+12+bdr,4).attr({fill:'#000000', stroke:'none'}));
    }
  }
  for (x=0;x<board_size;x++)
  {
    for (y=0;y<board_size;y++)
    {
      pos=xy2pos(x,y)
      c=data[pos];

      if (c=='B' || c=='W')
      {
        if (c=='B')
          gfx_cache.push(paper.circle(x*24+12+bdr,y*24+12+bdr,crad).attr({fill:'r(.3,.2)#555-#000'}));
        else if (c=='W')
          gfx_cache.push(paper.circle(x*24+12+bdr,y*24+12+bdr,crad).attr({fill:'r(.3,.2)#fff-#aaa',stroke:'#777'}));

        if ((c+":"+pos)==last_move)
          gfx_cache.push(paper.circle(x*24+12+bdr,y*24+12+bdr,4).attr({fill:'#7f7f7f',stroke:'none'}));
      }
      else if (pos==simple_ko)
      {
        gfx_cache.push(paper.rect(x*24+6+bdr,y*24+6+bdr,12,12));
      }
      else
      {
        itt=paper.rect(x*24+bdr,y*24+bdr,24,24).attr({fill:'#000',opacity:0});
        itt.data('pos',pos);
        itt.data('hvx',x*24+bdr+12);
        itt.data('hvy',y*24+bdr+12);
        itt.data('hvo',null);
        gfx_cache.push(itt);

        engine_to_play=engine_color=='both';
        if (next_color=='B' && engine_color=='black')
          engine_to_play=true;
        else if (next_color=='W' && engine_color=='white')
          engine_to_play=true;

        if (!game_over && !engine_to_play)
        {
          itt.click(function()
          {
            doGtpCmd('play',next_color+'&'+this.data('pos'),refreshBoard);
          });
          itt.mouseover(function()
          {
            x=this.data('hvx');
            y=this.data('hvy');
            hvo=paper.circle(x,y,crad).attr({stroke:'none',opacity:0.5});
            if (next_color=='B')
              hvo.attr({fill:'#000000'});
            else
              hvo.attr({fill:'#ffffff'});
            this.data('hvo',hvo);
            this.toFront();
          });
          itt.mouseout(function()
          {
            o=this.data('hvo');
            if (o!=null)
              o.remove();
          });
        }
      }
    }
  }
  
  if (game_over && board_scored!=null)
  {
    for (x=0;x<board_size;x++)
    {
      for (y=0;y<board_size;y++)
      {
        pos=xy2pos(x,y)
        c=board_scored[pos];
        if (c=='B')
          gfx_cache.push(paper.rect(x*24+bdr,y*24+bdr,24,24).attr({fill:'#000',stroke:'none',opacity:0.4}));
        else if (c=='W')
          gfx_cache.push(paper.rect(x*24+bdr,y*24+bdr,24,24).attr({fill:'#fff',stroke:'none',opacity:0.4}));
      }
    }
  }


  if (thinking)
  {
    gfx_cache.push(paper.rect(1,1,sz-2+2*bdr,sz-2+2*bdr).attr({fill:'#777',stroke:'none',opacity:0.4}));
    if (thinking)
    {
      for (i=0;i<spinnercount;i++)
      {
        spinners[i].toFront();
      }
    }
  }
  else
  {
    for (i=0;i<spinnercount;i++)
    {
      spinners[i].toBack();
    }
  }

  $('#page').show();
}

function moveDone()
{
  if (!game_over)
  {
    if (next_color=='B' && (engine_color=='black' || engine_color=='both'))
      doGtpCmdThink('genmove',next_color,refreshBoard);
    else if (next_color=='W' && (engine_color=='white' || engine_color=='both'))
      doGtpCmdThink('genmove',next_color,refreshBoard);
    else
    {
      thinking=false;
      enableButton('new');
      enableButton('pass');
      enableButton('genmove');
      if (moves>0)
        enableButton('undo');
      enableButton('settings');
      disableButton('stop');
      drawBoard();
    }
  }
  else
  {
    thinking=false;
    enableButton('new');
    if (moves>0)
      enableButton('undo');
    enableButton('settings');
    drawBoard();
  }
}

function updateStatus()
{
  info='';
  info+='Komi: '+komi+'<br/>\n';
  info+='Moves: '+moves+'<br/>\n';
  //info+='Last Move: '+last_move+'<br/>\n';
  /*if (moves>0 && last_move.split(':')[1]=='PASS')
  {
    if (last_move.split(':')[0]=='B')
      info+='Black';
    else
      info+='White';
    info+=' passed<br/>\n';
  }*/
  if (game_over)
    info+='Game finished<br/>\n';
  else
    info+='Next to Move: '+next_color+'<br/>\n';

  info+='Black prisoners: '+white_captures+'<br/>\n';
  info+='White prisoners: '+black_captures+'<br/>\n';
  info+='<span class="info">Note: Only area scoring is supported</span>';
  
  var drawcaptured=false;
  if (moves>0)
  {
    stat=moves+': ';
    if (last_move=='RESIGN')
      stat+=last_move;
    else
      stat+=last_move.split(':')[1];

    if (!game_over)
    {
      stat+=' <span class="pullright">';
      stat+=black_captures+'<span id="blackcaps" class="singlestone" title="Black stones captured by white"></span> ';
      stat+=white_captures+'<span id="whitecaps" class="singlestone" title="White stones captured by black"></span>';
      stat+='</span>';
      drawcaptured=true;
    }
  }
  else
    stat="Place a stone or click play to start";
  $('#status').html(stat);

  if (drawcaptured)
  {
    bc=Raphael('blackcaps',16,16);
    bc.circle(8,9,6).attr({fill:'r(.3,.2)#555-#000'});
    wc=Raphael('whitecaps',16,16);
    wc.circle(8,9,6).attr({fill:'r(.3,.2)#fff-#aaa',stroke:'#777'});
  }

  if (game_over)
  {
    if (last_move=='RESIGN')
    {
      info+='Result: '+next_color+'+R<br/>\n';
      $('#status').append(' - Result: '+next_color+'+R');
    }
    else
    {
      $.get('final_score.gtpcmd',function(data)
      {
        score=data.substr(2);
        if (score=='0')
          score='Jigo';
        info+='Result: '+score+'<br/>\n';
        $('#status').append(' - Result: '+score+' (Komi: '+komi+')');
        $.getJSON('board_scored.jsoncmd',function(data)
        {
          board_scored=data;
          drawBoard();
        });
      });
    }
  }
}

function refreshBoard()
{
  $.getJSON('board_info.jsoncmd',function(data)
  {
    if (data['size']!=board_size)
      size_chg=true;
    board_size=data['size'];
    komi=data['komi'];
    moves=data['moves'];
    last_move=data['last_move'];
    next_color=data['next_color'];
    simple_ko=data['simple_ko'];
    passes=data['passes'];
    threads=data['threads'];
    playouts=data['playouts'];
    time=data['time'];
    black_captures=data['black_captures'];
    white_captures=data['white_captures'];

    if (justundid)
    {
      if ((next_color=='B' && engine_color=='black') || (next_color=='W' && engine_color=='white'))
        engine_color='none';
      justundid=false;
    }

    if (passes>=2 || last_move=='RESIGN' || (passes==0 && moves>=2 && last_move.split(':')[1]=='PASS'))
      game_over=true;
    else
    {
      game_over=false;
      board_scored=null;
    }

    if (thinking || game_over)
    {
      disableButton('pass');
      disableButton('genmove');
      disableButton('undo');
    }
    else
    {
      enableButton('pass');
      enableButton('genmove');
      if (moves>0)
        enableButton('undo');
      else
        disableButton('undo');
    }
    
    if (thinking)
    {
      disableButton('new');
      disableButton('settings');
      enableButton('stop');
    }
    else
    {
      enableButton('new');
      enableButton('settings');
      disableButton('stop');
    }

    updateStatus();
    $.getJSON('board_pos.jsoncmd',function(data)
    {
      board_data=data;
      drawBoard();
      moveDone();
    });
  });
}

function updateParams()
{
  doGtpCmd('param','thread_count&'+threads,function(){
    doGtpCmd('param','playouts_per_move&'+playouts,function(){
      doGtpCmd('param','time_move_max&'+time,refreshBoard);
    });
  });
}

function initButton(name)
{
  r=Raphael(name,32,32);
  buttons[name]=r.path(icons[name]).attr({fill:'#222',stroke:'none',transform:'s0.8'});
  hvo=r.rect(0,0,32,32).attr({'fill':'#000','opacity':0});
  hvo.mouseover(function(){if (buttons_enabled[name]) buttons[name].attr('fill','#444');});
  hvo.mouseout(function(){if (buttons_enabled[name]) buttons[name].attr('fill','#222');});
  buttons_enabled[name]=true;
}

function initSmallButton(name)
{
  r=Raphael(name,16,16);
  buttons[name]=r.path(icons[name]).attr({fill:'#222',stroke:'none',transform:'s0.4t-16,-16'});
  hvo=r.rect(0,0,16,16).attr({'fill':'#000','opacity':0});
  hvo.mouseover(function(){if (buttons_enabled[name]) buttons[name].attr('fill','#444');});
  hvo.mouseout(function(){if (buttons_enabled[name]) buttons[name].attr('fill','#222');});
  buttons_enabled[name]=true;
}

function disableButton(name)
{
  buttons[name].attr('fill','#777');
  buttons_enabled[name]=false;
}

function enableButton(name)
{
  buttons[name].attr('fill','#222');
  buttons_enabled[name]=true;
}

$(document).ready(function()
{
  $('#page').hide(); // hide the content until the board has been drawn at least once
  r=Raphael('header',250,50);
  r.path(header).attr({fill:'#222',stroke:'none',transform:'s0.8t25,0'});

  $.getJSON('engine_info.jsoncmd',function(data)
  {
    $('#engine').html(data['name']+' '+data['version']);
  });
  
  /*$.getJSON('board_info.jsoncmd',function(data)
  {
    var items = [];

    $.each(data, function(key, val) {
      items.push('<li>' + key + ':' + val + '</li>');
    });

    $('<ul/>', {
      html: items.join('')
    }).appendTo('body');
  });*/
  
  initButton('new');
  initButton('pass');
  initButton('genmove');
  initButton('stop');
  initButton('info');
  initSmallButton('help');
  initButton('undo');
  initButton('settings');
  $('#page').tooltip();
  
  //$('#dialog-settings > tr').find('td:eq(1)').append('zzz');
  $('#dialog-settings tr').find('td:eq(0)').css('text-align','right');

  $('#dialog-new').dialog(
  {
    autoOpen: false,
    //height: 300,
    width: 250,
    modal: true,
    resizable: false,
    buttons: {
      'Cancel': function()
      {
        $(this).dialog('close');
      },
      'OK': function()
      {
        size_chg=true;
        engine_color=$('#dialog-new-color').val();
        
        enableButton('info');

        doGtpCmd('clear_board','',function(){
          doGtpCmd('boardsize',$('#dialog-new-size').val(),function(){
            doGtpCmd('komi',$('#dialog-new-komi').val(),refreshBoard);
          });
        });

        $(this).dialog('close');
      }
    }
  });
  $('#dialog-info').dialog(
  {
    autoOpen: false,
    width: 250,
    modal: true,
    resizable: false,
    buttons: {
      'OK': function()
      {
        $(this).dialog('close');
      }
    }
  });
  $('#dialog-help').dialog(
  {
    autoOpen: false,
    width: 500,
    modal: true,
    resizable: false,
    buttons: {
      'OK': function()
      {
        $(this).dialog('close');
      }
    }
  });
  $('#dialog-settings').dialog(
  {
    autoOpen: false,
    width: 300,
    modal: true,
    resizable: false,
    buttons: {
      'Cancel': function()
      {
        $(this).dialog('close');
      },
      'OK': function()
      {
        clearboard=false;
        if ($('#dialog-settings-size').val()!=size)
        {
          size=$('#dialog-settings-size').val();
          clearboard=true;
          size_chg=true;
        }
        if ($('#dialog-settings-komi').val()!=komi)
        {
          komi=$('#dialog-settings-komi').val();
          clearboard=true;
        }
        engine_color=$('#dialog-settings-color').val();
        threads=$('#dialog-settings-threads').val();
        if ($('#dialog-settings-playouts-en').attr('checked'))
          playouts=$('#dialog-settings-playouts').val();
        else
          playouts=10000000;
        if ($('#dialog-settings-time-en').attr('checked'))
          time=$('#dialog-settings-time').val();
        else
          time=3600;
        
        if (clearboard)
        {
          doGtpCmd('clear_board','',function(){
            doGtpCmd('boardsize',size,function(){
              doGtpCmd('komi',komi,updateParams);
            });
          });
        }
        else
          updateParams();

        $(this).dialog('close');
      }
    }
  });

  $('#new').click(function()
  {
    if (!buttons_enabled['new']) return;
    /*$('#dialog-new-size').val(board_size);
    $('#dialog-new-komi').val(komi);
    $('#dialog-new-color').val(engine_color);
    $('#dialog-new').dialog('open');*/
    doGtpCmd('clear_board','',refreshBoard);
  });
  $('#pass').click(function(){if (!buttons_enabled['pass']) return; doGtpCmd('play',next_color+'&pass',refreshBoard);});
  $('#genmove').click(function()
  {
    if (!buttons_enabled['genmove']) return;
    if (engine_color!='both')
    {
      if (next_color=='B')
        engine_color='black';
      else
        engine_color='white';
    }
    doGtpCmdThink('genmove',next_color,refreshBoard);
  });
  $('#stop').click(function()
  {
    if (!buttons_enabled['stop']) return;
    disableButton('stop');
    doGtpCmd('stop','',function()
    {
      engine_color='none'
      refreshBoard();
    });
  });
  $('#info').click(function()
  {
    if (!buttons_enabled['info']) return;
    $('#dialog-info-status').html(info);
    $('#dialog-info').dialog('open');
  });
  $('#help').click(function(){$('#dialog-help').dialog('open');});
  $('#settings').click(function()
  {
    if (!buttons_enabled['settings']) return;
    $('#dialog-settings-size').val(board_size);
    $('#dialog-settings-komi').val(komi);
    $('#dialog-settings-color').val(engine_color);
    $('#dialog-settings-threads').val(threads);
    if (playouts>=10000000)
    {
      $('#dialog-settings-playouts-en').attr('checked',false);
      $('#dialog-settings-playouts').val('unlimited');
      $('#dialog-settings-playouts').disabled=true;
      $('#dialog-settings-playouts').addClass('disabled');
    }
    else
    {
      $('#dialog-settings-playouts-en').attr('checked',true);
      $('#dialog-settings-playouts').val(playouts);
      $('#dialog-settings-playouts').disabled=false;
      $('#dialog-settings-playouts').removeClass('disabled');
    }
    if (time>=3600)
    {
      $('#dialog-settings-time-en').attr('checked',false);
      $('#dialog-settings-time').val('unlimited');
      $('#dialog-settings-time').disabled=true;
      $('#dialog-settings-time').addClass('disabled');
    }
    else
    {
      $('#dialog-settings-time-en').attr('checked',true);
      $('#dialog-settings-time').val(time);
      $('#dialog-settings-time').disabled=false;
      $('#dialog-settings-time').removeClass('disabled');
    }
    $('#dialog-settings').dialog('open');
  });
  $('#dialog-settings-playouts-en').change(function(){
    if (!$('#dialog-settings-playouts-en').attr('checked'))
    {
      $('#dialog-settings-playouts').val('unlimited');
      $('#dialog-settings-playouts').disabled=true;
      $('#dialog-settings-playouts').addClass('disabled');
    }
    else
    {
      $('#dialog-settings-playouts').val(playouts);
      $('#dialog-settings-playouts').disabled=false;
      $('#dialog-settings-playouts').removeClass('disabled');
    }
    return true;
  });
  $('#dialog-settings-time-en').change(function(){
    if (!$('#dialog-settings-time-en').attr('checked'))
    {
      $('#dialog-settings-time').val('unlimited');
      $('#dialog-settings-time').disabled=true;
      $('#dialog-settings-time').addClass('disabled');
    }
    else
    {
      $('#dialog-settings-time').val(time);
      $('#dialog-settings-time').disabled=false;
      $('#dialog-settings-time').removeClass('disabled');
    }
    return true;
  });
  $('#undo').click(function(){
    if (!buttons_enabled['undo']) return;
    doGtpCmd('undo','',function(){
      justundid=true;
      refreshBoard();
    });
  });

  refreshBoard();
});


