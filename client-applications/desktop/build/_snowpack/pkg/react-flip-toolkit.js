import { r as react } from './common/index-404563d3.js';
import './common/index-79331fc9.js';
import './common/_commonjsHelpers-798ad6a7.js';

/*! @license Rematrix v0.2.2

	Copyright 2018 Fisssion LLC.

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in
	all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
	THE SOFTWARE.
*/
/**
 * @module Rematrix
 */

/**
 * Transformation matrices in the browser come in two flavors:
 *
 *  - `matrix` using 6 values (short)
 *  - `matrix3d` using 16 values (long)
 *
 * This utility follows this [conversion guide](https://goo.gl/EJlUQ1)
 * to expand short form matrices to their equivalent long form.
 *
 * @param  {array} source - Accepts both short and long form matrices.
 * @return {array}
 */
function format(source) {
	if (source.constructor !== Array) {
		throw new TypeError('Expected array.')
	}
	if (source.length === 16) {
		return source
	}
	if (source.length === 6) {
		var matrix = identity();
		matrix[0] = source[0];
		matrix[1] = source[1];
		matrix[4] = source[2];
		matrix[5] = source[3];
		matrix[12] = source[4];
		matrix[13] = source[5];
		return matrix
	}
	throw new RangeError('Expected array with either 6 or 16 values.')
}

/**
 * Returns a matrix representing no transformation. The product of any matrix
 * multiplied by the identity matrix will be the original matrix.
 *
 * > **Tip:** Similar to how `5 * 1 === 5`, where `1` is the identity.
 *
 * @return {array}
 */
function identity() {
	var matrix = [];
	for (var i = 0; i < 16; i++) {
		i % 5 == 0 ? matrix.push(1) : matrix.push(0);
	}
	return matrix
}

/**
 * Returns a 4x4 matrix describing the combined transformations
 * of both arguments.
 *
 * > **Note:** Order is very important. For example, rotating 45°
 * along the Z-axis, followed by translating 500 pixels along the
 * Y-axis... is not the same as translating 500 pixels along the
 * Y-axis, followed by rotating 45° along on the Z-axis.
 *
 * @param  {array} m - Accepts both short and long form matrices.
 * @param  {array} x - Accepts both short and long form matrices.
 * @return {array}
 */
function multiply(m, x) {
	var fm = format(m);
	var fx = format(x);
	var product = [];

	for (var i = 0; i < 4; i++) {
		var row = [fm[i], fm[i + 4], fm[i + 8], fm[i + 12]];
		for (var j = 0; j < 4; j++) {
			var k = j * 4;
			var col = [fx[k], fx[k + 1], fx[k + 2], fx[k + 3]];
			var result =
				row[0] * col[0] + row[1] * col[1] + row[2] * col[2] + row[3] * col[3];

			product[i + k] = result;
		}
	}

	return product
}

/**
 * Attempts to return a 4x4 matrix describing the CSS transform
 * matrix passed in, but will return the identity matrix as a
 * fallback.
 *
 * **Tip:** In virtually all cases, this method is used to convert
 * a CSS matrix (retrieved as a `string` from computed styles) to
 * its equivalent array format.
 *
 * @param  {string} source - String containing a valid CSS `matrix` or `matrix3d` property.
 * @return {array}
 */
function parse(source) {
	if (typeof source === 'string') {
		var match = source.match(/matrix(3d)?\(([^)]+)\)/);
		if (match) {
			var raw = match[2].split(', ').map(parseFloat);
			return format(raw)
		}
	}
	return identity()
}

/**
 * Returns a 4x4 matrix describing X-axis scaling.
 *
 * @param  {number} scalar - Decimal multiplier.
 * @return {array}
 */
function scaleX(scalar) {
	var matrix = identity();
	matrix[0] = scalar;
	return matrix
}

/**
 * Returns a 4x4 matrix describing Y-axis scaling.
 *
 * @param  {number} scalar - Decimal multiplier.
 * @return {array}
 */
function scaleY(scalar) {
	var matrix = identity();
	matrix[5] = scalar;
	return matrix
}

/**
 * Returns a 4x4 matrix describing X-axis translation.
 *
 * @param  {number} distance - Measured in pixels.
 * @return {array}
 */
function translateX(distance) {
	var matrix = identity();
	matrix[12] = distance;
	return matrix
}

/**
 * Returns a 4x4 matrix describing Y-axis translation.
 *
 * @param  {number} distance - Measured in pixels.
 * @return {array}
 */
function translateY(distance) {
	var matrix = identity();
	matrix[13] = distance;
	return matrix
}

var a=function(t){return "number"==typeof t},o=function(t){return "function"==typeof t},l=function(t){return "[object Object]"===Object.prototype.toString.call(t)},p=function(t){return Array.prototype.slice.apply(t)},c$1=function(t){var e=t.reduce(function(t,e){return t[e]=(t[e]||0)+1,t},{});return Object.keys(e).filter(function(t){return e[t]>1})};function u$1(t){return [].slice.call(arguments,1).forEach(function(e){if(e)for(var i in e)Object.prototype.hasOwnProperty.call(e,i)&&(t[i]=e[i]);}),t}var d$1,f$1=function(t,e,i){return t+(e-t)*i},h$1={__proto__:null,isNumber:a,isFunction:o,isObject:l,toArray:p,getDuplicateValsAsStrings:c$1,assign:u$1,tweenProp:f$1},g={__proto__:null,DATA_FLIP_ID:"data-flip-id",DATA_INVERSE_FLIP_ID:"data-inverse-flip-id",DATA_FLIP_COMPONENT_ID:"data-flip-component-id",DATA_FLIP_CONFIG:"data-flip-config",DATA_PORTAL_KEY:"data-portal-key",DATA_EXIT_CONTAINER:"data-exit-container"},m$1={noWobble:{stiffness:200,damping:26},gentle:{stiffness:120,damping:14},veryGentle:{stiffness:130,damping:17},wobbly:{stiffness:180,damping:12},stiff:{stiffness:260,damping:26}},v=function(t){return l(t)?t:Object.keys(m$1).indexOf(t)>-1?m$1[t]:{}};"undefined"!=typeof window&&(d$1=window.requestAnimationFrame);var y=d$1=d$1||function(t){window.setTimeout(t,1e3/60);},_=Date.now(),S="object"==typeof performance&&"function"==typeof performance.now?function(){return performance.now()}:function(){return Date.now()-_};function E(t,e){var i=t.indexOf(e);-1!==i&&t.splice(i,1);}var A=function(){function t(){}return t.prototype.run=function(){var t=this;y(function(){t.springSystem.loop(S());});},t}(),C=function(){this.position=0,this.velocity=0;},b=0,I=function(){function t(t){this._id="s"+b++,this._springSystem=t,this.listeners=[],this._startValue=0,this._currentState=new C,this._displacementFromRestThreshold=.001,this._endValue=0,this._overshootClampingEnabled=!1,this._previousState=new C,this._restSpeedThreshold=.001,this._tempState=new C,this._timeAccumulator=0,this._wasAtRest=!0,this._cachedSpringConfig={};}var e=t.prototype;return e.getId=function(){return this._id},e.destroy=function(){this.listeners=[],this._springSystem.deregisterSpring(this);},e.setSpringConfig=function(t){return this._springConfig=t,this},e.getCurrentValue=function(){return this._currentState.position},e.getDisplacementDistanceForState=function(t){return Math.abs(this._endValue-t.position)},e.setEndValue=function(t){if(t===this._endValue)return this;if(this.prevEndValue=t,this._endValue===t&&this.isAtRest())return this;this._startValue=this.getCurrentValue(),this._endValue=t,this._springSystem.activateSpring(this.getId());for(var e=0,i=this.listeners.length;e<i;e++){var n=this.listeners[e].onSpringEndStateChange;n&&n(this);}return this},e.setVelocity=function(t){return t===this._currentState.velocity||(this._currentState.velocity=t,this._springSystem.activateSpring(this.getId())),this},e.setCurrentValue=function(t){this._startValue=t,this._currentState.position=t;for(var e=0,i=this.listeners.length;e<i;e++){var n=this.listeners[e];n.onSpringUpdate&&n.onSpringUpdate(this);}return this},e.setAtRest=function(){return this._endValue=this._currentState.position,this._tempState.position=this._currentState.position,this._currentState.velocity=0,this},e.setOvershootClampingEnabled=function(t){return this._overshootClampingEnabled=t,this},e.isOvershooting=function(){var t=this._startValue,e=this._endValue;return this._springConfig.tension>0&&(t<e&&this.getCurrentValue()>e||t>e&&this.getCurrentValue()<e)},e.advance=function(t,e){var i=this.isAtRest();if(!i||!this._wasAtRest){var n=e;e>.064&&(n=.064),this._timeAccumulator+=n;for(var r,s,a,o,l,p,c=this._springConfig.tension,u=this._springConfig.friction,d=this._currentState.position,f=this._currentState.velocity,h=this._tempState.position,g=this._tempState.velocity;this._timeAccumulator>=.001;)this._timeAccumulator-=.001,this._timeAccumulator<.001&&(this._previousState.position=d,this._previousState.velocity=f),s=c*(this._endValue-h)-u*f,o=c*(this._endValue-(h=d+.001*(r=f)*.5))-u*(g=f+.001*s*.5),p=c*(this._endValue-(h=d+.001*(a=g)*.5))-u*(g=f+.001*o*.5),h=d+.001*(l=g),d+=1/6*(r+2*(a+l)+(g=f+.001*p))*.001,f+=1/6*(s+2*(o+p)+(c*(this._endValue-h)-u*g))*.001;this._tempState.position=h,this._tempState.velocity=g,this._currentState.position=d,this._currentState.velocity=f,this._timeAccumulator>0&&this._interpolate(this._timeAccumulator/.001),(this.isAtRest()||this._overshootClampingEnabled&&this.isOvershooting())&&(this._springConfig.tension>0?(this._startValue=this._endValue,this._currentState.position=this._endValue):(this._endValue=this._currentState.position,this._startValue=this._endValue),this.setVelocity(0),i=!0);var m=!1;this._wasAtRest&&(this._wasAtRest=!1,m=!0);var v=!1;i&&(this._wasAtRest=!0,v=!0),this.notifyPositionUpdated(m,v);}},e.notifyPositionUpdated=function(t,e){var i=this;this.listeners.filter(Boolean).forEach(function(n){t&&n.onSpringActivate&&!i._onActivateCalled&&(n.onSpringActivate(i),i._onActivateCalled=!0),n.onSpringUpdate&&n.onSpringUpdate(i),e&&n.onSpringAtRest&&n.onSpringAtRest(i);});},e.systemShouldAdvance=function(){return !this.isAtRest()||!this.wasAtRest()},e.wasAtRest=function(){return this._wasAtRest},e.isAtRest=function(){return Math.abs(this._currentState.velocity)<this._restSpeedThreshold&&(this.getDisplacementDistanceForState(this._currentState)<=this._displacementFromRestThreshold||0===this._springConfig.tension)},e._interpolate=function(t){this._currentState.position=this._currentState.position*t+this._previousState.position*(1-t),this._currentState.velocity=this._currentState.velocity*t+this._previousState.velocity*(1-t);},e.addListener=function(t){return this.listeners.push(t),this},e.addOneTimeListener=function(t){var e=this;return Object.keys(t).forEach(function(i){var n;t[i]=(n=t[i],function(){n.apply(void 0,[].slice.call(arguments)),e.removeListener(t);});}),this.listeners.push(t),this},e.removeListener=function(t){return E(this.listeners,t),this},t}(),O=function(){function t(t){this.looper=t||new A,this.looper.springSystem=this,this.listeners=[],this._activeSprings=[],this._idleSpringIndices=[],this._isIdle=!0,this._lastTimeMillis=-1,this._springRegistry={};}var e=t.prototype;return e.createSpring=function(t,e){return this.createSpringWithConfig({tension:t,friction:e})},e.createSpringWithConfig=function(t){var e=new I(this);return this.registerSpring(e),e.setSpringConfig(t),e},e.getIsIdle=function(){return this._isIdle},e.registerSpring=function(t){this._springRegistry[t.getId()]=t;},e.deregisterSpring=function(t){E(this._activeSprings,t),delete this._springRegistry[t.getId()];},e.advance=function(t,e){for(var i=this;this._idleSpringIndices.length>0;)this._idleSpringIndices.pop();for(this._activeSprings.filter(Boolean).forEach(function(n){n.systemShouldAdvance()?n.advance(t/1e3,e/1e3):i._idleSpringIndices.push(i._activeSprings.indexOf(n));});this._idleSpringIndices.length>0;){var n=this._idleSpringIndices.pop();n>=0&&this._activeSprings.splice(n,1);}},e.loop=function(t){var e;-1===this._lastTimeMillis&&(this._lastTimeMillis=t-1);var i=t-this._lastTimeMillis;this._lastTimeMillis=t;var n=0,r=this.listeners.length;for(n=0;n<r;n++)(e=this.listeners[n]).onBeforeIntegrate&&e.onBeforeIntegrate(this);for(this.advance(t,i),0===this._activeSprings.length&&(this._isIdle=!0,this._lastTimeMillis=-1),n=0;n<r;n++)(e=this.listeners[n]).onAfterIntegrate&&e.onAfterIntegrate(this);this._isIdle||this.looper.run();},e.activateSpring=function(t){var e=this._springRegistry[t];-1===this._activeSprings.indexOf(e)&&this._activeSprings.push(e),this.getIsIdle()&&(this._isIdle=!1,this.looper.run());},t}(),w=new O,x=function(t){var e=t.springConfig,i=e.overshootClamping,n=t.getOnUpdateFunc,r=t.onAnimationEnd,s=t.onSpringActivate,a=w.createSpring(e.stiffness,e.damping);a.setOvershootClampingEnabled(!!i);var o={onSpringActivate:s,onSpringAtRest:function(){a.destroy(),r();},onSpringUpdate:n({spring:a,onAnimationEnd:r})};return a.addListener(o),a},U=function(t){var e=x(t);return e.setEndValue(1),e},V=function(t,e){if(void 0===e&&(e={}),t&&t.length){e.reverse&&t.reverse();var i,n="number"!=typeof(i=e.speed)?1.1:1+Math.min(Math.max(5*i,0),5),r=1/Math.max(Math.min(t.length,100),10),s=t.map(function(t,e){var i=t.getOnUpdateFunc;return t.getOnUpdateFunc=function(t){var a=i(t);return function(t){var i=t.getCurrentValue();(i=i<.01?0:i>.99?1:i)>=r&&s[e+1]&&s[e+1](Math.max(Math.min(i*n,1),0)),a(t);}},t}).map(function(t){var e=x(t);if(e)return e.setEndValue.bind(e)}).filter(Boolean);s[0]&&s[0](1);}},F=function(t){return [0,1,4,5,12,13].map(function(e){return t[e]})},P=function(t){return t.top<window.innerHeight&&t.bottom>0&&t.left<window.innerWidth&&t.right>0};function R(t){return JSON.parse(t.dataset.flipConfig||"{}")}var T=function(t,e){var i;return u$1(t,((i={})[e[0]]=e[1],i))},D=function(t,e){return p(e?document.querySelectorAll('[data-portal-key="'+e+'"]'):t.querySelectorAll("[data-flip-id]"))},M=function(t){return t.map(function(t){return [t,t.getBoundingClientRect()]})},k=function(l){var d,h=l.cachedOrderedFlipIds,g=void 0===h?[]:h,y=l.inProgressAnimations,_=void 0===y?{}:y,S=l.flippedElementPositionsBeforeUpdate,E=void 0===S?{}:S,A=l.flipCallbacks,C=void 0===A?{}:A,b=l.containerEl,I=l.applyTransformOrigin,O=l.spring,w=l.debug,x=l.portalKey,k=l.staggerConfig,j=void 0===k?{}:k,B=l.decisionData,N=void 0===B?{}:B,L=l.handleEnterUpdateDelete,q=l.onComplete,W=l.onStart,K=M(D((d={element:b,portalKey:x}).element,d.portalKey)).map(function(t){var e=t[0],i=t[1],n=window.getComputedStyle(e);return [e.dataset.flipId,{element:e,rect:i,opacity:parseFloat(n.opacity),transform:n.transform}]}).reduce(T,{}),X=function(t){var e=t.containerEl,i=t.portalKey;return i?function(t){return function(e){return p(document.querySelectorAll('[data-portal-key="'+t+'"]'+e))}}(i):e?function(t){var e=Math.random().toFixed(5);return t.dataset.flipperId=e,function(i){return p(t.querySelectorAll('[data-flipper-id="'+e+'"] '+i))}}(e):function(){return []}}({containerEl:b,portalKey:x}),Y=function(t){return function(e){return t('[data-flip-id="'+e+'"]')[0]}}(X),H=function(t){return E[t]&&K[t]},J=Object.keys(E).concat(Object.keys(K)).filter(function(t){return !H(t)}),z={flipCallbacks:C,getElement:Y,flippedElementPositionsBeforeUpdate:E,flippedElementPositionsAfterUpdate:K,inProgressAnimations:_,decisionData:N},G=function(t){var e,i=t.unflippedIds,n=t.flipCallbacks,r=t.getElement,s=t.flippedElementPositionsBeforeUpdate,a=t.flippedElementPositionsAfterUpdate,o=t.inProgressAnimations,l=t.decisionData,p=i.filter(function(t){return a[t]}).filter(function(t){return n[t]&&n[t].onAppear}),c=i.filter(function(t){return s[t]&&n[t]&&n[t].onExit}),u=new Promise(function(t){e=t;}),d=[],f=0,h=c.map(function(t,i){var r=s[t].domDataForExitAnimations,a=r.element,p=r.parent,c=r.childPosition,u=c.top,h=c.left,g=c.width,m=c.height;"static"===getComputedStyle(p).position&&(p.style.position="relative"),a.style.transform="matrix(1, 0, 0, 1, 0, 0)",a.style.position="absolute",a.style.top=u+"px",a.style.left=h+"px",a.style.height=m+"px",a.style.width=g+"px";var v=d.filter(function(t){return t[0]===p})[0];v||(v=[p,document.createDocumentFragment()],d.push(v)),v[1].appendChild(a),f+=1;var y=function(){try{p.removeChild(a);}catch(t){}finally{0==(f-=1)&&e();}};return o[t]={stop:y},function(){return n[t].onExit(a,i,y,l)}});return d.forEach(function(t){t[0].appendChild(t[1]);}),h.length||e(),{hideEnteringElements:function(){p.forEach(function(t){var e=r(t);e&&(e.style.opacity="0");});},animateEnteringElements:function(){p.forEach(function(t,e){var i=r(t);i&&n[t].onAppear(i,e,l);});},animateExitingElements:function(){return h.forEach(function(t){return t()}),u}}}(u$1({},z,{unflippedIds:J})),Q=G.hideEnteringElements,Z=G.animateEnteringElements,$=G.animateExitingElements,tt=u$1({},z,{containerEl:b,flippedIds:g.filter(H),applyTransformOrigin:I,spring:O,debug:w,staggerConfig:j,scopedSelector:X,onComplete:q});W&&W(b,N);var et=function(l){var d,h=l.flippedIds,g=l.flipCallbacks,y=l.inProgressAnimations,_=l.flippedElementPositionsBeforeUpdate,S=l.flippedElementPositionsAfterUpdate,E=l.applyTransformOrigin,A=l.spring,C=l.getElement,b=l.debug,I=l.staggerConfig,O=void 0===I?{}:I,w=l.decisionData,x=void 0===w?{}:w,T=l.onComplete,D=l.containerEl,M=new Promise(function(t){d=t;});if(T&&M.then(function(){return T(D,x)}),!h.length)return function(){return d([]),M};var k=[],j=C(h[0]),B=j?j.ownerDocument.querySelector("body"):document.querySelector("body"),N=(c$1(h),h.map(function(l){var c=_[l].rect,h=S[l].rect,C=_[l].opacity,b=S[l].opacity,I=h.width<1||h.height<1,O=S[l].element;if(!P(c)&&!P(h))return !1;if(!O)return !1;var w,U,V,T=R(O),D=(V=(U=void 0===(w={flipperSpring:A,flippedSpring:T.spring})?{}:w).flippedSpring,u$1({},m$1.noWobble,v(U.flipperSpring),v(V))),M=!0===T.stagger?"default":T.stagger,j={element:O,id:l,stagger:M,springConfig:D};if(g[l]&&g[l].shouldFlip&&!g[l].shouldFlip(x.previous,x.current))return !1;var L=Math.abs(c.left-h.left)+Math.abs(c.top-h.top),q=Math.abs(c.width-h.width)+Math.abs(c.height-h.height),W=Math.abs(b-C);if(0===c.height&&0===h.height||0===c.width&&0===h.width||L<.5&&q<.5&&W<.01)return !1;var K=parse(S[l].transform),X={matrix:K},Y={matrix:[]},H=[K];T.translate&&(H.push(translateX(c.left-h.left)),H.push(translateY(c.top-h.top))),T.scale&&(H.push(scaleX(Math.max(c.width,1)/Math.max(h.width,1))),H.push(scaleY(Math.max(c.height,1)/Math.max(h.height,1)))),T.opacity&&(Y.opacity=C,X.opacity=b);var J=[];if(!g[l]||!g[l].shouldInvert||g[l].shouldInvert(x.previous,x.current)){var z=function(t,e){return p(t.querySelectorAll('[data-inverse-flip-id="'+e+'"]'))}(O,l);J=z.map(function(t){return [t,R(t)]});}Y.matrix=F(H.reduce(multiply)),X.matrix=F(X.matrix);var G,Q=function(t){var e=t.element,i=t.invertedChildren,n=t.body;return function(t){var r=t.matrix,s=t.opacity,o=t.forceMinVals;if(a(s)&&(e.style.opacity=s+""),o&&(e.style.minHeight="1px",e.style.minWidth="1px"),r){var l=function(t){return "matrix("+t.join(", ")+")"}(r);e.style.transform=l,i&&function(t){var e=t.matrix,i=t.body;t.invertedChildren.forEach(function(t){var n=t[0],r=t[1];if(i.contains(n)){var s=e[0],a=e[3],o=e[5],l={translateX:0,translateY:0,scaleX:1,scaleY:1},p="";r.translate&&(l.translateX=-e[4]/s,l.translateY=-o/a,p+="translate("+l.translateX+"px, "+l.translateY+"px)"),r.scale&&(l.scaleX=1/s,l.scaleY=1/a,p+=" scale("+l.scaleX+", "+l.scaleY+")"),n.style.transform=p;}});}({invertedChildren:i,matrix:r,body:n});}}}({element:O,invertedChildren:J,body:B});if(g[l]&&g[l].onComplete){var Z=g[l].onComplete;G=function(){return Z(O,x)};}var $=a(Y.opacity)&&a(X.opacity)&&Y.opacity!==X.opacity,tt=!1;return u$1({},j,{stagger:M,springConfig:D,getOnUpdateFunc:function(t){var e=t.spring,i=t.onAnimationEnd;return y[l]={destroy:e.destroy.bind(e),onAnimationEnd:i},function(t){g[l]&&g[l].onSpringUpdate&&g[l].onSpringUpdate(t.getCurrentValue()),tt||(tt=!0,g[l]&&g[l].onStart&&g[l].onStart(O,x));var e=t.getCurrentValue();if(B.contains(O)){var i={matrix:[]};i.matrix=Y.matrix.map(function(t,i){return f$1(t,X.matrix[i],e)}),$&&(i.opacity=f$1(Y.opacity,X.opacity,e)),Q(i);}else t.destroy();}},initializeFlip:function(){Q({matrix:Y.matrix,opacity:$?Y.opacity:void 0,forceMinVals:I}),g[l]&&g[l].onStartImmediate&&g[l].onStartImmediate(O,x),T.transformOrigin?O.style.transformOrigin=T.transformOrigin:E&&(O.style.transformOrigin="0 0"),J.forEach(function(t){var e=t[0],i=t[1];i.transformOrigin?e.style.transformOrigin=i.transformOrigin:E&&(e.style.transformOrigin="0 0");});},onAnimationEnd:function(t){delete y[l],o(G)&&G(),O.style.transform="",J.forEach(function(t){t[0].style.transform="";}),I&&O&&(O.style.minHeight="",O.style.minWidth=""),t||(k.push(l),k.length>=N.length&&d(k));},delayUntil:T.delayUntil})}).filter(Boolean));if(N.forEach(function(t){return (0, t.initializeFlip)()}),b)return function(){};var L=N.filter(function(t){return t.delayUntil&&(e=t.delayUntil,N.filter(function(t){return t.id===e}).length);var e;}),q={},W={},K={};L.forEach(function(t){t.stagger?(K[t.stagger]=!0,W[t.delayUntil]?W[t.delayUntil].push(t.stagger):W[t.delayUntil]=[t.stagger]):q[t.delayUntil]?q[t.delayUntil].push(t):q[t.delayUntil]=[t];});var X=N.filter(function(t){return t.stagger}).reduce(function(t,e){return t[e.stagger]?t[e.stagger].push(e):t[e.stagger]=[e],t},{}),Y=N.filter(function(t){return -1===L.indexOf(t)});return Y.forEach(function(t){t.onSpringActivate=function(){q[t.id]&&q[t.id].forEach(U),W[t.id]&&Object.keys(W[t.id].reduce(function(t,e){var i;return u$1(t,((i={})[e]=!0,i))},{})).forEach(function(t){V(X[t],O[t]);});};}),function(){return N.length||d([]),Y.filter(function(t){return !t.stagger}).forEach(U),Object.keys(X).forEach(function(t){K[t]||V(X[t],O[t]);}),M}}(tt);L?L({hideEnteringElements:Q,animateEnteringElements:Z,animateExitingElements:$,animateFlippedElements:et}):(Q(),$().then(Z),et());},j=function(t){var e=t.element,i=t.flipCallbacks,n=void 0===i?{}:i,r=t.inProgressAnimations,s=void 0===r?{}:r,a=D(e,t.portalKey),o=p(e.querySelectorAll("[data-inverse-flip-id]")),l={},c=[],d={};a.filter(function(t){return n&&n[t.dataset.flipId]&&n[t.dataset.flipId].onExit}).forEach(function(t){var e=t.parentNode;if(t.closest){var i=t.closest("[data-exit-container]");i&&(e=i);}var n=c.findIndex(function(t){return t[0]===e});-1===n&&(c.push([e,e.getBoundingClientRect()]),n=c.length-1),l[t.dataset.flipId]=c[n][1],d[t.dataset.flipId]=e;});var f=M(a),h=f.map(function(t){var e=t[0],i=t[1],r={};if(n&&n[e.dataset.flipId]&&n[e.dataset.flipId].onExit){var s=l[e.dataset.flipId];u$1(r,{element:e,parent:d[e.dataset.flipId],childPosition:{top:i.top-s.top,left:i.left-s.left,width:i.width,height:i.height}});}return [e.dataset.flipId,{rect:i,opacity:parseFloat(window.getComputedStyle(e).opacity||"1"),domDataForExitAnimations:r}]}).reduce(T,{});return function(t,e){Object.keys(t).forEach(function(e){t[e].destroy&&t[e].destroy(),t[e].onAnimationEnd&&t[e].onAnimationEnd(!0),delete t[e];}),e.forEach(function(t){t.style.transform="",t.style.opacity="";});}(s,a.concat(o)),{flippedElementPositions:h,cachedOrderedFlipIds:f.map(function(t){return t[0].dataset.flipId})}};new O;

function c(e,t){if(null==e)return {};var r,n,i={},o=Object.keys(e);for(n=0;n<o.length;n++)t.indexOf(r=o[n])>=0||(i[r]=e[r]);return i}var d=react.createContext({}),f=react.createContext("portal"),u=function(t){var r,n;function i(){var e;return (e=t.apply(this,arguments)||this).inProgressAnimations={},e.flipCallbacks={},e.el=void 0,e}n=t,(r=i).prototype=Object.create(n.prototype),r.prototype.constructor=r,r.__proto__=n;var l=i.prototype;return l.getSnapshotBeforeUpdate=function(e){return e.flipKey!==this.props.flipKey&&this.el?j({element:this.el,flipCallbacks:this.flipCallbacks,inProgressAnimations:this.inProgressAnimations,portalKey:this.props.portalKey}):null},l.componentDidUpdate=function(e,t,r){this.props.flipKey!==e.flipKey&&this.el&&k({flippedElementPositionsBeforeUpdate:r.flippedElementPositions,cachedOrderedFlipIds:r.cachedOrderedFlipIds,containerEl:this.el,inProgressAnimations:this.inProgressAnimations,flipCallbacks:this.flipCallbacks,applyTransformOrigin:this.props.applyTransformOrigin,spring:this.props.spring,debug:this.props.debug,portalKey:this.props.portalKey,staggerConfig:this.props.staggerConfig,handleEnterUpdateDelete:this.props.handleEnterUpdateDelete,decisionData:{previous:e.decisionData,current:this.props.decisionData},onComplete:this.props.onComplete,onStart:this.props.onStart});},l.render=function(){var t=this,r=this.props,n=r.portalKey,i=react.createElement(d.Provider,{value:this.flipCallbacks},react.createElement(r.element,{className:r.className,ref:function(e){return t.el=e}},this.props.children));return n&&(i=react.createElement(f.Provider,{value:n},i)),i},i}(react.Component);u.defaultProps={applyTransformOrigin:!0,element:"div"};var h=function(e){var t,r=e.children,o=e.flipId,p=e.inverseFlipId,s=e.portalKey,d=c(e,["children","flipId","inverseFlipId","portalKey"]),f=r,u=function(e){return "function"==typeof e}(f);if(!u)try{f=react.Children.only(r);}catch(e){throw new Error("Each Flipped component must wrap a single child")}d.scale||d.translate||d.opacity||h$1.assign(d,{translate:!0,scale:!0,opacity:!0});var h=((t={})[g.DATA_FLIP_CONFIG]=JSON.stringify(d),t);return void 0!==o?h[g.DATA_FLIP_ID]=String(o):p&&(h[g.DATA_INVERSE_FLIP_ID]=String(p)),void 0!==s&&(h[g.DATA_PORTAL_KEY]=s),u?f(h):react.cloneElement(f,h)},m=function(t){var r=t.children,n=t.flipId,i=t.shouldFlip,o=t.shouldInvert,p=t.onAppear,a=t.onStart,s=t.onStartImmediate,u=t.onComplete,m=t.onExit,g=t.onSpringUpdate,y=c(t,["children","flipId","shouldFlip","shouldInvert","onAppear","onStart","onStartImmediate","onComplete","onExit","onSpringUpdate"]);return r?y.inverseFlipId?react.createElement(h,Object.assign({},y),r):react.createElement(f.Consumer,null,function(t){return react.createElement(d.Consumer,null,function(c){return h$1.isObject(c)&&n&&(c[n]={shouldFlip:i,shouldInvert:o,onAppear:p,onStart:a,onStartImmediate:s,onComplete:u,onExit:m,onSpringUpdate:g}),react.createElement(h,Object.assign({flipId:n},y,{portalKey:t}),r)})}):null};m.displayName="Flipped";

export { m as Flipped, u as Flipper };
