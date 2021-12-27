import React, { Fragment } from "react";
import { Dialog, Transition } from "@headlessui/react";

import { omnibarContext } from "@app/renderer/context/omnibar";

const Overlay = (props: { contents: JSX.Element | JSX.Element[] }) => {
  const context = omnibarContext();
  return (
    <Transition appear show={context.showOverlay} as={Fragment}>
      <Dialog
        as="div"
        className="fixed inset-0 z-10 overflow-y-auto"
        onClose={context.setShowOverlay}
      >
        <div className="min-h-screen px-4 text-center">
          <Transition.Child
            as={Fragment}
            enter="ease-out duration-75"
            enterFrom="opacity-0"
            enterTo="opacity-100"
            leave="ease-in duration-75"
            leaveFrom="opacity-100"
            leaveTo="opacity-0"
          >
            <Dialog.Overlay className="fixed inset-0" />
          </Transition.Child>

          {/* This element is to trick the browser into centering the modal contents. */}
          <span
            className="inline-block h-screen align-middle"
            aria-hidden="true"
          >
            &#8203;
          </span>
          <Transition.Child
            as={Fragment}
            enter="ease-out duration-300"
            enterFrom="opacity-0 scale-95"
            enterTo="opacity-100 scale-100"
            leave="ease-in duration-200"
            leaveFrom="opacity-100 scale-100"
            leaveTo="opacity-0 scale-95"
          >
            <div className="inline-block w-full max-w-md overflow-hidden text-right align-middle transition-all transform">
              <div className="flex flex-col space-y-2 text-gray-300 mt-4 max-h-32 overflow-y-scroll">
                {props.contents}
              </div>
            </div>
          </Transition.Child>
        </div>
      </Dialog>
    </Transition>
  );
};

export default Overlay;
