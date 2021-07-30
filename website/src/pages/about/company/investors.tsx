import React from "react"

const people = [
  {
    name: "Slow Ventures",
    role: "Venture Capital",
    imageUrl:
      "https://media-exp1.licdn.com/dms/image/C4D0BAQFW2KzutcS3XA/company-logo_200_200/0/1519910041267?e=1635379200&v=beta&t=P8Z9euNU6WIQesKQLdu6KbJ2tsJ9_uEBCdzZCvEOco0",
  },
  {
    name: "Neo",
    role: "Venture Capital",
    imageUrl:
      "https://media-exp1.licdn.com/dms/image/C560BAQGZOQ8pLmLpDQ/company-logo_200_200/0/1589995692628?e=1635379200&v=beta&t=XU2KNhYgiDpK-e75tcMnBSgDhbdL4p1VASny8ga0HiY",
  },
  {
    name: "Basis Set Ventures",
    role: "Venture Capital",
    imageUrl:
      "https://media-exp1.licdn.com/dms/image/C4D0BAQHb0p21AeBPGw/company-logo_200_200/0/1519922057504?e=1635379200&v=beta&t=ClSIF4c0_ibu9q4Y99PDbdVchRuVfKG__zrcn-aFZGA",
  },
  {
    name: "Rough Draft Ventures",
    role: "Venture Capital",
    imageUrl:
      "https://media-exp1.licdn.com/dms/image/C4D0BAQHqWFIqhd_3ZQ/company-logo_200_200/0/1519904523552?e=1635379200&v=beta&t=3tDgSvrqDMC61oFFyEPdIv2iyRgP-hEIq2GvA_NY0UE",
  },
  {
    name: "DRF",
    role: "Venture Capital",
    imageUrl:
      "https://media-exp1.licdn.com/dms/image/C560BAQG6PkyvkmtgqQ/company-logo_200_200/0/1600364918804?e=1635379200&v=beta&t=IgmeRTIlHxtDipnx71BPLoaGRbVYMOja83d_dL3dqxU",
  },
  {
    name: "Pankaj Patel",
    role: "Former Cisco Chief Development Officer",
    imageUrl:
      "https://media-exp1.licdn.com/dms/image/C4E03AQFmCx91DOS4Ew/profile-displayphoto-shrink_400_400/0/1516160124284?e=1632960000&v=beta&t=A3zvNkzeHpih-CEpUfZRKWrlFj0MGy9xd7ig9pM1FnI",
  },
  {
    name: "Michael Stoppelman",
    role: "Former Yelp SVP of Engineering",
    imageUrl:
      "https://media-exp1.licdn.com/dms/image/C5603AQHKydSZhT8fSw/profile-displayphoto-shrink_400_400/0/1574370350594?e=1632960000&v=beta&t=AmcH5yZ8aVO3umHAddrL1xqc0z083FX2Bp7Od7wAVYI",
  },
  {
    name: "Vijay Pandurangan",
    role: "Former Twitter Director of Engineering",
    imageUrl:
      "https://media-exp1.licdn.com/dms/image/C5603AQGv7lP3Ngi1tA/profile-displayphoto-shrink_400_400/0/1516621671570?e=1632960000&v=beta&t=FqJbiF9wHKqpdgbcuxTwrrYcQcfLdKaO18rPGuFIf2M",
  },
]

const Investors = () => {
  return (
    <div className="mx-auto py-12 max-w-7xl lg:py-24">
      <div className="grid grid-cols-1 gap-12 lg:grid-cols-3 lg:gap-8">
        <div className="space-y-5 sm:space-y-4 pr-6">
          <div className="text-3xl text-gray-300 sm:text-5xl">
            Meet our investors
          </div>
          <p className="text-xl text-gray-500">
            Fractal is backed by renowned institutional investors and angels.
          </p>
        </div>
        <div className="lg:col-span-2">
          <ul className="space-y-12 sm:grid sm:grid-cols-2 sm:gap-12 sm:space-y-0 lg:gap-x-8">
            {people.map((person) => (
              <li key={person.name}>
                <div className="flex items-center space-x-4 lg:space-x-6">
                  <img
                    className="w-16 h-16 rounded-full border-2 filter grayscale mr-4"
                    src={person.imageUrl}
                    alt=""
                  />
                  <div className="font-medium leading-6 space-y-1 text-gray-400">
                    <div className="text-xl">{person.name}</div>
                    <p className="text-gray-500 text-sm">{person.role}</p>
                  </div>
                </div>
              </li>
            ))}
          </ul>
        </div>
      </div>
    </div>
  )
}

export default Investors
