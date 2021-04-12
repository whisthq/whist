import { m as mergeAll, f as internalFromArray, p as popScheduler, o as operate, g as OperatorSubscriber, e as innerFrom } from '../common/mergeAll-683720e5.js';

function concatAll() {
    return mergeAll(1);
}

function concat() {
    var args = [];
    for (var _i = 0; _i < arguments.length; _i++) {
        args[_i] = arguments[_i];
    }
    return concatAll()(internalFromArray(args, popScheduler(args)));
}

function mapTo(value) {
    return operate(function (source, subscriber) {
        source.subscribe(new OperatorSubscriber(subscriber, function () { return subscriber.next(value); }));
    });
}

function startWith() {
    var values = [];
    for (var _i = 0; _i < arguments.length; _i++) {
        values[_i] = arguments[_i];
    }
    var scheduler = popScheduler(values);
    return operate(function (source, subscriber) {
        (scheduler ? concat(values, source, scheduler) : concat(values, source)).subscribe(subscriber);
    });
}

function switchMap(project, resultSelector) {
    return operate(function (source, subscriber) {
        var innerSubscriber = null;
        var index = 0;
        var isComplete = false;
        var checkComplete = function () { return isComplete && !innerSubscriber && subscriber.complete(); };
        source.subscribe(new OperatorSubscriber(subscriber, function (value) {
            innerSubscriber === null || innerSubscriber === void 0 ? void 0 : innerSubscriber.unsubscribe();
            var innerIndex = 0;
            var outerIndex = index++;
            innerFrom(project(value, outerIndex)).subscribe((innerSubscriber = new OperatorSubscriber(subscriber, function (innerValue) { return subscriber.next(resultSelector ? resultSelector(value, innerValue, outerIndex, innerIndex++) : innerValue); }, undefined, function () {
                innerSubscriber = null;
                checkComplete();
            })));
        }, undefined, function () {
            isComplete = true;
            checkComplete();
        }));
    });
}

export { mapTo, startWith, switchMap };
